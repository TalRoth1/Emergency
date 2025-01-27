package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;


public class StompConnections<T> implements Connections<T> {
   // connectionId -> ConnectionHandler<T>
   private final ConcurrentHashMap<Integer, ConnectionHandler<T>> clients;
   
   // channel -> set of connectionIds
   private final ConcurrentHashMap<String, CopyOnWriteArraySet<Integer>> topicSubscribers;
   
   // connectionId -> (subscriptionId -> channel)
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
    public void send(String channel, T msg)
    {
        for (Integer connId : topicSubscribers.getOrDefault(channel, new CopyOnWriteArraySet<>()))
        {
            send(connId, msg);
            System.out.println("Sent message to " + connId);
        }
    }
    @Override
    public void disconnect(int connectionId)
    {
        clients.remove(connectionId);
        for (CopyOnWriteArraySet<Integer> subs : topicSubscribers.values()) {
            subs.remove(connectionId);
        }
        subscriptions.remove(connectionId);
        System.out.println("Disconnected " + connectionId); 
    }

    public void addClient(int connectionId, ConnectionHandler<T> handler)
    {
        clients.put(connectionId, handler);
    }

    public void subscribe(String channel, int connectionId, String subId)
    {
        topicSubscribers.computeIfAbsent(channel, k -> new CopyOnWriteArraySet<>()).add(connectionId);
        subscriptions.computeIfAbsent(connectionId, k -> new ConcurrentHashMap<>()).put(subId, channel);
        System.out.println("Subscribed " + connectionId + " to " + channel);        
    }

    public void unsubscribe(String subId, int connectionId)
    {
        ConcurrentHashMap<String, String> subMap = subscriptions.getOrDefault(connectionId, new ConcurrentHashMap<>());
        String channel = subMap.remove(subId);
        if (channel != null) 
        {
            topicSubscribers.getOrDefault(channel, new CopyOnWriteArraySet<>()).remove(connectionId);
        }
        System.out.println("Unsubscribed " + connectionId + " from " + channel);
    }
    public boolean broadcast(String channel, T message, int connectionId)
    {
        if(!topicSubscribers.containsKey(channel) || !topicSubscribers.get(channel).contains(connectionId))
        {
            return false;
        }
        for (Integer connId : topicSubscribers.getOrDefault(channel, new CopyOnWriteArraySet<>()))
        {
            send(connId, message);
        }
        return true;
    }
    
    public int size() 
    {
        return clients.size();
    }
}
