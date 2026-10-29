// SPDX-License-Identifier: GPL-2.0-only
import javax.microedition.midlet.MIDlet;
import javax.microedition.media.*;
import javax.microedition.media.control.VolumeControl;
import java.io.ByteArrayInputStream;

/** Exercises the 27-player WAV cache pattern found in Doodle Jump 1.8.2. */
public final class AudioSmoke extends MIDlet implements Runnable {
    protected void startApp() { new Thread(this).start(); }
    protected void pauseApp() {}
    protected void destroyApp(boolean unconditional) {}

    private static void put(byte[] bytes, int at, int value, int count) {
        for (int i = 0; i < count; ++i) bytes[at + i] = (byte)(value >>> (8 * i));
    }

    private static byte[] makeWave(int samples) {
        byte[] bytes = new byte[44 + samples * 2];
        byte[] header = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' '};
        System.arraycopy(header, 0, bytes, 0, header.length);
        put(bytes, 4, bytes.length - 8, 4);
        put(bytes, 16, 16, 4);
        put(bytes, 20, 1, 2);
        put(bytes, 22, 1, 2);
        put(bytes, 24, 22050, 4);
        put(bytes, 28, 44100, 4);
        put(bytes, 32, 2, 2);
        put(bytes, 34, 16, 2);
        bytes[36] = 'd'; bytes[37] = 'a'; bytes[38] = 't'; bytes[39] = 'a';
        put(bytes, 40, samples * 2, 4);
        for (int i = 0; i < samples; ++i)
            put(bytes, 44 + i * 2, (i % 50 < 25 ? 2400 : -2400), 2);
        return bytes;
    }

    public void run() {
        Player[] cache = new Player[27];
        try {
            byte[] sample = makeWave(22050);
            for (int i = 0; i < cache.length; ++i) {
                cache[i] = Manager.createPlayer(new ByteArrayInputStream(sample), "audio/x-wav");
                cache[i].realize();
                ((VolumeControl)cache[i].getControl("VolumeControl")).setLevel(5);
            }
            for (int i = 0; i < cache.length; ++i) cache[i].start();
            Thread.sleep(150);
            for (int i = 0; i < cache.length; ++i)
                if (cache[i].getState() != Player.STARTED)
                    throw new RuntimeException("cached WAV player failed to start: " + i);
            System.out.println("[PS3ME AUDIO TEST] 27 cached WAV players PASS");
            for (int i = 0; i < cache.length; ++i) cache[i].close();

            Player rewind = Manager.createPlayer(new ByteArrayInputStream(sample), "audio/x-wav");
            rewind.realize();
            rewind.start();
            Thread.sleep(600);
            if (rewind.getState() != Player.STARTED)
                throw new RuntimeException("WAV ended before rewind");
            rewind.setMediaTime(0);
            Thread.sleep(600);
            if (rewind.getState() != Player.STARTED)
                throw new RuntimeException("active WAV rewind did not restart playback");
            Thread.sleep(700);
            if (rewind.getState() == Player.STARTED)
                throw new RuntimeException("rewound WAV Player did not finish");
            rewind.close();
            System.out.println("[PS3ME AUDIO TEST] active WAV rewind PASS");

            byte[] midi = {'M','T','h','d',0,0,0,6,0,0,0,1,0,96,
                           'M','T','r','k',0,0,0,12,0,(byte)0x90,60,100,
                           96,(byte)0x80,60,0,0,(byte)0xff,47,0};
            Player music = Manager.createPlayer(new ByteArrayInputStream(midi), "audio/midi");
            music.realize();
            music.start();
            Thread.sleep(100);
            if (music.getState() != Player.STARTED)
                throw new RuntimeException("MIDI note did not start");
            Thread.sleep(1400);
            music.close();
            System.out.println("[PS3ME AUDIO TEST] MIDI smoke PASS");
        } catch (Throwable failure) {
            System.out.println("[PS3ME AUDIO TEST] FAIL " + failure);
            failure.printStackTrace();
        } finally {
            for (int i = 0; i < cache.length; ++i)
                if (cache[i] != null) try { cache[i].close(); } catch (Throwable ignored) {}
        }
    }
}
