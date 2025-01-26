package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.SocketChannel;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class NonBlockingConnectionHandler<T> implements ConnectionHandler<T> {

    private static final int BUFFER_ALLOCATION_SIZE = 1 << 13; // 8k
    private static final ConcurrentLinkedQueue<ByteBuffer> BUFFER_POOL = new ConcurrentLinkedQueue<>();

    private final MessagingProtocol<T> protocol;
    private final MessageEncoderDecoder<T> encdec;
    private final Queue<ByteBuffer> writeQueue = new ConcurrentLinkedQueue<>();
    private final SocketChannel chan;
    private final Reactor<T> reactor;

    public NonBlockingConnectionHandler(MessageEncoderDecoder<T> reader, MessagingProtocol<T> protocol, SocketChannel chan, Reactor<T> reactor) {
        this.chan = chan;
        this.encdec = reader;
        this.protocol = protocol;
        this.reactor = reactor;
    }

    public Runnable continueRead() {
        ByteBuffer buf = leaseBuffer();
        boolean success = false;

        try {
            int bytesRead = chan.read(buf);
            if (bytesRead == -1) {
                System.out.println("Client disconnected: " + chan);
                success = false;
            } else if (bytesRead > 0) {
                success = true;
            }
        } catch (IOException ex) {
            System.err.println("IOException in continueRead: " + ex.getMessage());
        }

        if (success) {
            buf.flip();
            return () -> {
                try {
                    while (buf.hasRemaining()) {
                        T nextMessage = encdec.decodeNextByte(buf.get());
                        if (nextMessage != null) {
                            try {
                                protocol.process(nextMessage);
                                if (protocol.shouldTerminate()) {
                                    close();
                                }
                            } catch (Exception e) {
                                System.err.println("Error in protocol.process: " + e.getMessage());
                                close();
                            }
                        }
                    }
                } finally {
                    releaseBuffer(buf);
                }
            };
        } else {
            releaseBuffer(buf);
            close();
            return null;
        }
    }

    public void continueWrite() {
        while (!writeQueue.isEmpty()) {
            try {
                ByteBuffer top = writeQueue.peek();
                chan.write(top);
                if (top.hasRemaining()) {
                    return; // Partial write, stop here and wait for next write opportunity
                } else {
                    writeQueue.remove(); // Remove fully written buffer
                }
            } catch (IOException ex) {
                System.err.println("IOException in continueWrite: " + ex.getMessage());
                close();
                return;
            }
        }

        if (protocol.shouldTerminate()) {
            System.out.println("Protocol requested termination; closing connection...");
            close();
        } else {
            reactor.updateInterestedOps(chan, SelectionKey.OP_READ);
        }
    }

    @Override
    public void send(T msg) {
        if (msg != null) {
            try {
                byte[] bytes = encdec.encode(msg);
                ByteBuffer buffer = ByteBuffer.wrap(bytes);
                writeQueue.add(buffer);
                reactor.updateInterestedOps(chan, SelectionKey.OP_READ | SelectionKey.OP_WRITE);
            } catch (Exception e) {
                System.err.println("Error in send(): " + e.getMessage());
            }
        }
    }

    public void close() {
        try {
            System.out.println("Closing connection for: " + chan);
            chan.close();
        } catch (IOException ex) {
            System.err.println("IOException during close: " + ex.getMessage());
        }
    }

    public boolean isClosed() {
        return !chan.isOpen();
    }

    private static ByteBuffer leaseBuffer() {
        ByteBuffer buff = BUFFER_POOL.poll();
        if (buff == null) {
            return ByteBuffer.allocateDirect(BUFFER_ALLOCATION_SIZE);
        }
        buff.clear();
        return buff;
    }

    private static void releaseBuffer(ByteBuffer buff) {
        BUFFER_POOL.add(buff);
    }
}
