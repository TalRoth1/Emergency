 package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.StompMessagingProtocolimp;
import bgu.spl.net.srv.StompEncoderDecoder;
import bgu.spl.net.srv.Server;
import bgu.spl.net.srv.frame;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.err.println("Usage: StompServer <port> <tpc|reactor>");
            return;
        }

        int port = Integer.parseInt(args[0]);
        String serverType = args[1]; // "tpc" or "reactor"

        Server<frame> server;
        if (serverType.equalsIgnoreCase("tpc")) {
            server = Server.threadPerClient(port, StompMessagingProtocolimp::new, StompEncoderDecoder::new);
        } else if (serverType.equalsIgnoreCase("reactor")) {
            server = Server.reactor(
                    Runtime.getRuntime().availableProcessors(),
                    port,
                    StompMessagingProtocolimp::new,
                    StompEncoderDecoder::new
            );
        } else {
            throw new IllegalArgumentException("Invalid server type: " + serverType);
        }
        System.out.println("Starting server on port " + port + " using " + serverType.toUpperCase());
        server.serve();
    }
}
