package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;


public class StompConnections<T> implements Connections<T> {
    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> clients;
    private final ConcurrentHashMap<String, CopyOnWriteArraySet<Integer>> topicSubscribers;
    private final ConcurrentHashMap<String, ConcurrentHashMap<String, CopyOnWriteArraySet<String>>> messages;


    public StompConnections() {
        clients = new ConcurrentHashMap<>();
        topicSubscribers = new ConcurrentHashMap<>();
        messages= new ConcurrentHashMap<>();
    }

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = clients.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
        CopyOnWriteArraySet<Integer> subscribers = topicSubscribers.get(channel);
        if (subscribers != null) {
            for (Integer connectionId : subscribers) {
                send(connectionId, msg);
            }
        }
    }

    @Override
    public void disconnect(int connectionId) {
        clients.remove(connectionId);
        // Remove the client from all subscribed topics
        for (CopyOnWriteArraySet<Integer> subscribers : topicSubscribers.values()) {
            subscribers.remove(connectionId);
        }
    }

    public void addClient(int connectionId, ConnectionHandler<T> handler) {
        clients.put(connectionId, handler);
    }

    public void subscribe(String channel, int connectionId) {
        topicSubscribers.computeIfAbsent(channel, k -> new CopyOnWriteArraySet<>()).add(connectionId);
    }

    public void unsubscribe(String channel, int connectionId) {
        CopyOnWriteArraySet<Integer> subscribers = topicSubscribers.get(channel);
        if (subscribers != null) {
            subscribers.remove(connectionId);
        }
    }

    public void saveMessage(String channel, String user, String message) {
        messages
            .computeIfAbsent(channel, k -> new ConcurrentHashMap<>())
            .computeIfAbsent(user, k -> new CopyOnWriteArraySet<>())
            .add(message);
    }

    public CopyOnWriteArraySet<String> getMessages(String channel, String user) {
        return messages.getOrDefault(channel, new ConcurrentHashMap<>()).getOrDefault(user, new CopyOnWriteArraySet<>());
    
    }    
    public int size() {
        return clients.size();
    }
}
