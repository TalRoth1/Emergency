package bgu.spl.net.api;

import java.util.Arrays;

public class StompEncoderDecoder implements MessageEncoderDecoder<String>{
    private byte[] bytes = new byte[1 << 10]; //start with 1k
    private int len = 0;
    private String result = null;

    @Override
    public String decodeNextByte(byte nextByte)
    {
        if (nextByte == '\u0000') {
            result = new String(bytes, 0, len, java.nio.charset.StandardCharsets.UTF_8);
            len = 0;
            return result;
        }
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }
        bytes[len++] = nextByte;
        return null;
    }

    @Override
    public byte[] encode(String message) {
        return (message + '\u0000').getBytes(java.nio.charset.StandardCharsets.UTF_8);
    }
}
