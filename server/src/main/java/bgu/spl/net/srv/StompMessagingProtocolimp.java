package bgu.spl.net.srv;

import bgu.spl.net.api.MessagingProtocol;

import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolimp implements MessagingProtocol<String> {

    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;
    private static ConcurrentHashMap<String, String> users = new ConcurrentHashMap<>();

    @Override
    public void start(int connectionId, Connections<String> connections)
    {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public String process(String message)
    {
        String[] lines = message.split("\n");
        String command = lines[0];
        switch (command) {
            case "CONNECT":
                return handleConnect(lines);
            case "SUBSCRIBE":
                return handleSubscribe(lines);
            case "UNSUBSCRIBE":
                return handleUnsubscribe(lines);
            case "SEND":
                return handleSend(lines);
            case "DISCONNECT":
                return handleDisconnect(lines);
            default:
                return createErrorFrame("Unknown command");
        }
    }

    @Override
    public boolean shouldTerminate() 
    {
        return shouldTerminate;
    }

    private String handleConnect(String[] lines)
    {
        String login = null, passcode = null;
        for (String line : lines) {
            if (line.startsWith("login:")) login = line.substring(6);
            if (line.startsWith("passcode:")) passcode = line.substring(9);
        }
        if (login == null || passcode == null) {
            return createErrorFrame("Missing login or passcode");
        }
        if (!users.containsKey(login)) {
            users.put(login, passcode);
            return "CONNECTED\nversion:1.2\n\n";
        } else if (!users.get(login).equals(passcode)) {
            return createErrorFrame("Wrong password");
        }
        return "CONNECTED\nversion:1.2\n\n";
    }

    private String handleSubscribe(String[] lines) {
        String destination = null, id = null;
        for (String line : lines) {
            if (line.startsWith("destination:")) destination = line.substring(12);
            if (line.startsWith("id:")) id = line.substring(3);
        }
        if (destination == null || id == null) {
            return createErrorFrame("Missing destination or id");
        }
        connections.subscribe(destination, connectionId);
        return "RECEIPT\nreceipt-id:" + id + "\n\n";
    }

    private String handleUnsubscribe(String[] lines)
    {
        String id = null;
        for (String line : lines) {
            if (line.startsWith("id:")) id = line.substring(3);
        }
        if (id == null) {
            return createErrorFrame("Missing id");
        }
        connections.unsubscribe(id, connectionId);
        return "RECEIPT\nreceipt-id:" + id + "\n\n";
    }

    private String handleSend(String[] lines) {
        String destination = null, body = null;
        for (String line : lines) {
            if (line.startsWith("destination:")) destination = line.substring(12);
            if (!line.contains(":")) body = line;
        }
        if (destination == null || body == null) {
            return createErrorFrame("Missing destination or body");
        }
        connections.send(destination, body);
        return null; // No response needed
    }

    private String handleDisconnect(String[] lines) {
        String receiptId = null;
        for (String line : lines) {
            if (line.startsWith("receipt:")) receiptId = line.substring(8);
        }
        shouldTerminate = true;
        connections.disconnect(connectionId);
        return "RECEIPT\nreceipt-id:" + receiptId + "\n\n";
    }

    private String createErrorFrame(String message) {
        return "ERROR\nmessage:" + message + "\n\n";
    }
}