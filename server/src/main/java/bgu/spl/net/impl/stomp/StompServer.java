package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompEncoderDecoder;
import bgu.spl.net.srv.Server;
import bgu.spl.net.srv.StompMessagingProtocolimp;

public class StompServer {

    public static void main(String[] args) {
        int port = Integer.parseInt(args[0]);
        String serverType = args[1]; // "tpc" or "reactor"

        Server<String> server;
        if (serverType.equalsIgnoreCase("tpc")) {
            server = Server.threadPerClient(port, StompMessagingProtocolimp::new, StompEncoderDecoder::new);
        } 
        else if (serverType.equalsIgnoreCase("reactor"))
        {
            server = Server.reactor(Runtime.getRuntime().availableProcessors(), port, StompMessagingProtocolimp::new, StompEncoderDecoder::new);
        } else {
            throw new IllegalArgumentException("Invalid server type");
        }
        server.serve();
    }
}