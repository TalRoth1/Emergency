package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;


public class StompConnections<T> implements Connections<T> {
    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> clients;
    private final ConcurrentHashMap<String, CopyOnWriteArraySet<Integer>> topicSubscribers;

    public StompConnections() {
        clients = new ConcurrentHashMap<>();
        topicSubscribers = new ConcurrentHashMap<>();
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
    public int size() {
        return clients.size();
    }
}
