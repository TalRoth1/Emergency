package bgu.spl.net.srv;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import bgu.spl.net.api.MessageEncoderDecoder;

public class StompEncoderDecoder implements MessageEncoderDecoder<frame>{
    private byte[] bytes = new byte[1 << 10]; //start with 1k
    private int len = 0;
    

    @Override
    public frame decodeNextByte(byte nextByte)
    {
        if (nextByte == '\u0000') {
            String frameString = new String(bytes, 0, len, StandardCharsets.UTF_8);
            len = 0;
            return frame.fromString(frameString);
        }
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }
        bytes[len++] = nextByte;
        return null;
    }

    @Override
    public byte[] encode(frame message) {
        String raw = message.toString() + "\u0000";
        return raw.getBytes(StandardCharsets.UTF_8);

    }
}
