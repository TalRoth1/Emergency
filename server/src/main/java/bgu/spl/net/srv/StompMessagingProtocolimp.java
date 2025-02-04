package bgu.spl.net.srv;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.api.frame;

import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolimp implements StompMessagingProtocol<frame> {

    private int connectionId;
    private Connections<frame> connections;
    private boolean shouldTerminate = false;

    // user -> password
    private static ConcurrentHashMap<String, String> users = new ConcurrentHashMap<>();
    // connectionId -> username
    private static ConcurrentHashMap<Integer, String> activeUsers = new ConcurrentHashMap<>();

    @Override
    public void start(int connectionId, Connections<frame> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(frame msg) {
        if (msg == null) {
            // Malformed frame or parse error
            connections.send(connectionId, createErrorFrame("Malformed frame received", ""));
            activeUsers.remove(connectionId);
            return;
        }

        String command = msg.getCommand();
        switch (command) {
            case "CONNECT":
                handleConnect(msg);
                break;
            case "SUBSCRIBE":
                handleSubscribe(msg);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(msg);
                break;
            case "SEND":
                handleSend(msg);
                break;
            case "DISCONNECT":
                handleDisconnect(msg);
                break;
            default:
            {
                connections.send(connectionId, createErrorFrame("Unknown command: " + command, ""));
                activeUsers.remove(connectionId);
            }
        }
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    // ------------------ Command Handlers ------------------

    private void handleConnect(frame msg) {
        String login = msg.getHeader("login");
        String passcode = msg.getHeader("passcode");

        if (login == null || passcode == null) {
            connections.send(connectionId, createErrorFrame("Missing login or passcode", ""));
            activeUsers.remove(connectionId);
            return;
        }
        // If new user, add to map
        if(activeUsers.contains(login))
        {
            connections.send(connectionId, createErrorFrame("User already logged in", ""));
            activeUsers.remove(connectionId);
            return;
        }
        else if (!users.containsKey(login))
            users.put(login, passcode);
        else
        {
            // existing user, check password
            if (!users.get(login).equals(passcode))
            {
                connections.send(connectionId, createErrorFrame("Wrong password", ""));
                activeUsers.remove(connectionId);
                return;
            }
        }

        // Mark user as active
        activeUsers.put(connectionId, login);

        // Return CONNECTED
        frame connected = new frame();
        connected.setCommand("CONNECTED");
        connected.addHeader("version", "1.2");
        connected.setBody(""); // no body
        if(connections!= null)
            connections.send(connectionId, connected);
        System.out.println(connectionId);
    }

    private void handleSubscribe(frame msg) 
    {
        String destination = msg.getHeader("destination");
        String subId = msg.getHeader("id");
        String receiptId = msg.getHeader("receipt");
        if (destination == null || subId == null)
        {
            connections.send(connectionId, createErrorFrame("Missing 'destination' or 'id' in SUBSCRIBE", receiptId));
            activeUsers.remove(connectionId);
            return;
        }

        // Record subscription
        connections.subscribe(destination, connectionId, subId);

        // Return a RECEIPT frame

        if (receiptId != null)
        {
            frame receipt = new frame();
            receipt.setCommand("RECEIPT");
            receipt.addHeader("receipt-id", receiptId);
            receipt.setBody("");
            connections.send(connectionId, receipt);
        }
    }

    private void handleUnsubscribe(frame msg) {
        String subId = msg.getHeader("id");
        String receiptId = msg.getHeader("receipt");
        if (subId == null) 
        {
            connections.send(connectionId, createErrorFrame("Missing 'id' in UNSUBSCRIBE", receiptId));
            activeUsers.remove(connectionId);
            return;
        }

        connections.unsubscribe(subId, connectionId);


        if(receiptId != null)
        {
            // Return a RECEIPT frame
            frame receipt = new frame();
            receipt.setCommand("RECEIPT");
            receipt.addHeader("receipt-id", receiptId);
            receipt.setBody("");
            connections.send(connectionId, receipt);
        }
    }

    private void handleSend(frame msg) {
        String destination = msg.getHeader("destination");
        if (destination == null) 
        {
            connections.send(connectionId, createErrorFrame("Missing 'destination' in SEND", ""));
            activeUsers.remove(connectionId);
            return;
        }

        // broadcast to all subscribers
        // We'll build a MESSAGE frame for each subscriber
        // But for simplicity, we can just reuse the body from the original frame
        // and define a single message frame that is broadcast
        // If you need per-subscriber subscription ID, you can do that in StompConnections.

        frame messageToBroadcast = new frame();
        messageToBroadcast.setCommand("MESSAGE");
        messageToBroadcast.addHeader("destination", destination);
        String user = activeUsers.get(connectionId);
        if (user != null)
            messageToBroadcast.addHeader("user", user);
        // Set body from the original frame
        messageToBroadcast.setBody(msg.getBody());

        // Now broadcast
        if(!(((StompConnections<frame>) connections).broadcast(destination.substring(1), messageToBroadcast, connectionId)))
        {
            connections.send(connectionId, createErrorFrame("You must be subscribed to the channel", ""));
            activeUsers.remove(connectionId);
        }

        // If there's a receipt
        String receipt = msg.getHeader("receipt");
        if (receipt != null) {
            frame receiptFrame = new frame();
            receiptFrame.setCommand("RECEIPT");
            receiptFrame.addHeader("receipt-id", receipt);
            connections.send(connectionId, receiptFrame);
        }
    }

    private void handleDisconnect(frame msg) {
        String receipt = msg.getHeader("receipt");
        
        // Return RECEIPT if needed
        if (receipt != null) {
            frame receiptFrame = new frame();
            receiptFrame.setCommand("RECEIPT");
            receiptFrame.addHeader("receipt-id", receipt);
            connections.send(connectionId, receiptFrame);
        }
        // remove from active
        activeUsers.remove(connectionId);
        connections.disconnect(connectionId);
    }

    // ------------------ Helper Methods ------------------

    private frame createErrorFrame(String message, String receipt) {
        frame error = new frame();
        error.setCommand("ERROR");
        error.addHeader("receiptId", receipt);
        error.addHeader("message", message);
        error.setBody("The server has caught an error:\n" + message);
        return error;
    }
}
