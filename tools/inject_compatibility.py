#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile, preverify and atomically package the platform's vendor APIs."""
from pathlib import Path
import argparse, os, subprocess, tempfile, zipfile

ROOT = Path(__file__).resolve().parents[1]

def prepare_manager():
    from patch_utils import replace
    manager=ROOT/'deps/phoneME/midp/src/media/reference/classes/javax/microedition/media/Manager.java'
    replace(manager,'            throw new IllegalArgumentException("bad param");\n        }\n',
            '            throw new IllegalArgumentException("bad param");\n        }\n'
            '        com.ps3me.media.TonePlayer.playTone(note,duration,volume);\n')
    text=manager.read_text()
    start=text.index('    public static Player createPlayer(String locator)')
    end=text.index('\n    }',start)+len('\n    }')
    text=text[:start]+'''    public static Player createPlayer(String locator) throws IOException, MediaException {
        if(locator==null)throw new IllegalArgumentException();
        if(TONE_DEVICE_LOCATOR.equals(locator))return new com.ps3me.media.TonePlayer();
        throw new MediaException("Cannot create Player");
    }'''+text[end:]
    start=text.index('    public static Player createPlayer(InputStream stream, String type)')
    end=text.index('\n\n\n    /**',start)
    text=text[:start]+'''    public static Player createPlayer(InputStream stream, String type) throws IOException, MediaException {
        if(stream==null)throw new IllegalArgumentException();
        if ("audio/x-tone-seq".equals(type)) return com.ps3me.media.TonePlayer.fromStream(stream);
        return new com.ps3me.media.AudioPlayer(stream,type);
    }'''+text[end:]
    start=text.index('    public static String[] getSupportedContentTypes(String protocol)')
    end=text.index('\n    }',start)+len('\n    }')
    text=text[:start]+'''    public static String[] getSupportedContentTypes(String protocol) {
        if("device".equals(protocol))return new String[]{"audio/x-tone-seq"};
        if(protocol==null)return new String[]{"audio/x-wav","audio/wav","audio/midi","audio/x-midi","audio/mpeg","audio/mp3","audio/x-tone-seq"};
        return new String[0];
    }'''+text[end:]
    start=text.index('    public static String[] getSupportedProtocols(String content_type)')
    end=text.index('\n    }',start)+len('\n    }')
    text=text[:start]+'''    public static String[] getSupportedProtocols(String content_type) {
        if(content_type==null || "audio/x-tone-seq".equals(content_type))return new String[]{"device"};
        return new String[0];
    }'''+text[end:]
    manager.write_text(text)
    return manager

def inject(jdk, runner=None):
    library = ROOT / 'build/midp_ppu/classes.zip'
    sources = sorted((ROOT / 'upstream/ps2/vm/api/nokia').glob('*.java'))
    sources += sorted((ROOT / 'ps3/java/com/nokia/mid/sound').glob('*.java'))
    # Sound is adapted to the PS3 MMAPI backend. Never compile PS2 SPU2 natives.
    sources += [ROOT / 'upstream/ps2/vm/api/nokia/sound/SoundListener.java']
    sources += [ROOT/'ps3/java/com/ps3me/media/AudioPlayer.java',ROOT/'ps3/java/com/ps3me/media/ToneSequence.java',
                ROOT/'ps3/java/com/ps3me/media/TonePlayer.java', prepare_manager()]
    with tempfile.TemporaryDirectory(prefix='vendor-api-', dir=library.parent) as tmp:
        tmp = Path(tmp); raw = tmp / 'raw'; verified = tmp / 'verified'
        raw.mkdir(); verified.mkdir()
        subprocess.run([str(jdk/'bin/javac'), '-source', '1.4', '-target', '1.4',
                        '-bootclasspath', str(library), '-d', str(raw)] +
                       [str(s) for s in sources], check=True)
        preverify = ROOT / 'deps/phoneME/preverifier/build/linux/preverify'
        names = [p.relative_to(raw).with_suffix('').as_posix().replace('/', '.')
                 for p in sorted(raw.rglob('*.class'))]
        for name in names:
            command = ([str(runner)] if runner else []) + [str(preverify),
                       '-classpath', str(raw)+os.pathsep+str(library),
                       '-d', str(verified), name]
            subprocess.run(command, check=True)
        entries = {p.relative_to(verified).as_posix(): p.read_bytes()
                   for p in verified.rglob('*.class')}
        expected = {name.replace('.', '/')+'.class' for name in names}
        if set(entries) != expected:
            raise RuntimeError('Preverification omitted platform classes')
        staged = tmp / 'classes.zip'
        with zipfile.ZipFile(library) as old, zipfile.ZipFile(staged, 'w', zipfile.ZIP_DEFLATED) as new:
            seen = set()
            for info in old.infolist():
                if info.filename in seen:
                    raise RuntimeError('Duplicate runtime entry: '+info.filename)
                seen.add(info.filename)
                if info.filename not in entries:
                    new.writestr(info, old.read(info))
            for name, data in sorted(entries.items()):
                new.writestr(name, data)
        with zipfile.ZipFile(staged) as check:
            assert check.testzip() is None
            assert expected.issubset(check.namelist())
        staged.replace(library)
    print('Packaged %d preverified Nokia platform classes' % len(names))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jdk', type=Path, required=True)
    parser.add_argument('--runner', type=Path)
    args = parser.parse_args()
    inject(args.jdk.resolve(), args.runner.resolve() if args.runner else None)
