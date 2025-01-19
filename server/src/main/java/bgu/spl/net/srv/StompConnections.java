package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;


public class StompConnections<T> implements Connections<T> {
    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> clients;
    private final ConcurrentHashMap<String, CopyOnWriteArraySet<Integer>> topicSubscribers;
    private final ConcurrentHashMap<Integer, ConcurrentHashMap<String, String>> subscriptions;


    public StompConnections() {
        clients = new ConcurrentHashMap<>();
        topicSubscribers = new ConcurrentHashMap<>();
        subscriptions = new ConcurrentHashMap<>();
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
        for (Integer connId : topicSubscribers.getOrDefault(channel, new CopyOnWriteArraySet<>())) {
            send(connId, msg);
        }
    }
    
    public void sendChanel(String destination, String body)
    {}
    
    @Override
    public void disconnect(int connectionId) {
        clients.remove(connectionId);
        for (CopyOnWriteArraySet<Integer> subscribers : topicSubscribers.values()) {
            subscribers.remove(connectionId);
        }
        subscriptions.remove(connectionId);
    }

    public void addClient(int connectionId, ConnectionHandler<T> handler) {
        clients.put(connectionId, handler);
    }

    public void subscribe(String channel, int connectionId, String subId) {
        topicSubscribers.computeIfAbsent(channel, k -> new CopyOnWriteArraySet<>()).add(connectionId);
        subscriptions.computeIfAbsent(connectionId, k -> new ConcurrentHashMap<>()).put(subId, channel);
    }

    public void unsubscribe(String subId, int connectionId) {
        String channel = subscriptions.getOrDefault(connectionId, new ConcurrentHashMap<>()).remove(subId);
        if (channel != null) {
            topicSubscribers.getOrDefault(channel, new CopyOnWriteArraySet<>()).remove(connectionId);
        }
    }
    public void broadcast(String channel, T message) {
        for (Integer connId : topicSubscribers.getOrDefault(channel, new CopyOnWriteArraySet<>())) {
            send(connId, message);
        }
    }
    
    public int size() {
        return clients.size();
    }

    public void saveMessage(String destination, String user, String body)
    {
    }
}
