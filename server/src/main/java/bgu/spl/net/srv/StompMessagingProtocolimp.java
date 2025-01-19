package bgu.spl.net.srv;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.StompMessagingProtocol;

import java.io.IOError;
import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolimp implements StompMessagingProtocol<String> {

    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;
    private static ConcurrentHashMap<String, String> users = new ConcurrentHashMap<>();
    private static ConcurrentHashMap<Integer, String> activeUsers = new ConcurrentHashMap<>(); // connectionId -> username



    @Override
    public void start(int connectionId, Connections<String> connections)
    {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(String message)
    {
        String[] lines = message.split("\n");
        String command = lines[0];
        switch (command) {
            case "CONNECT":
                handleConnect(lines);
                break;
            case "SUBSCRIBE":
                handleSubscribe(lines);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(lines);
                break;
            case "SEND":
                handleSend(lines);
                break;
            case "DISCONNECT":
                handleDisconnect(lines);
                break;
            default:
                connections.send(connectionId, createErrorFrame("Unknown command"));
        }
    }

    @Override
    public boolean shouldTerminate() 
    {
        return shouldTerminate;
    }

    private void handleConnect(String[] lines)
    {
        try
        {
            String login = getHeader(lines, "login");
            String passcode = getHeader(lines, "passcode");
            if (login == null || passcode == null) {
                connections.send(connectionId, createErrorFrame("Missing login or passcode"));
                return;
            }
    
            if (!users.containsKey(login)) {
                users.put(login, passcode);
            } else if (!users.get(login).equals(passcode)) {
                connections.send(connectionId, createErrorFrame("Wrong password"));
                return;
            }
    
            activeUsers.put(connectionId, login);
            connections.send(connectionId, "CONNECTED\nversion:1.2\n\n\0");
        }
    }

    private void handleSubscribe(String[] lines) {
        String destination = getHeader(lines, "destination");
        String subId = getHeader(lines, "id");

        if (destination == null || subId == null) {
            connections.send(connectionId, createErrorFrame("Missing destination or id in SUBSCRIBE"));
            return;
        }

        ((StompConnections<String>) connections).subscribe(destination, connectionId, subId);
        connections.send(connectionId, "RECEIPT\nreceipt-id:" + subId + "\n\n\0");
    }

    private void handleUnsubscribe(String[] lines) {
        String subId = getHeader(lines, "id");

        if (subId == null) {
            connections.send(connectionId, createErrorFrame("Missing id in UNSUBSCRIBE"));
            return;
        }

        ((StompConnections<String>) connections).unsubscribe(subId, connectionId);
        connections.send(connectionId, "RECEIPT\nreceipt-id:" + subId + "\n\n\0");
    }


    private void handleSend(String[] lines) {
        String destination = getHeader(lines, "destination");
        String body = extractBody(lines);

        if (destination == null || body.isEmpty()) {
            connections.send(connectionId, createErrorFrame("Missing destination or body in SEND"));
            return;
        }

        // Broadcast to all subscribers of the channel
        ((StompConnections<String>) connections).broadcast(destination, body);

        // If the frame includes a receipt header
        String receipt = getHeader(lines, "receipt");
        if (receipt != null) {
            connections.send(connectionId, "RECEIPT\nreceipt-id:" + receipt + "\n\n\0");
        }
    }
    private void handleDisconnect(String[] lines) {
        String receipt = getHeader(lines, "receipt");
        shouldTerminate = true;

        activeUsers.remove(connectionId);
        connections.disconnect(connectionId);

        if (receipt != null) {
            connections.send(connectionId, "RECEIPT\nreceipt-id:" + receipt + "\n\n\0");
        }
    }
    private String getHeader(String[] lines, String header) {
        for (String line : lines) {
            if (line.startsWith(header + ":")) {
                return line.substring((header + ":").length()).trim();
            }
        }
        return null;
    }

    private String extractBody(String[] lines) {
        boolean inBody = false;
        StringBuilder body = new StringBuilder();
        for (String line : lines) {
            if (inBody) {
                body.append(line).append("\n");
            } else if (line.isEmpty()) {
                inBody = true;
            }
        }
        return body.toString().trim();
    }

    private String createErrorFrame(String message) {
        return "ERROR\nmessage:" + message + "\n\n\0";
    }
}