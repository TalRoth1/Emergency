package bgu.spl.net.srv;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void sendChanel(String destination, String body); // Added

    void disconnect(int connectionId);

    void subscribe(String dest, int connectionId, String subId); // Added

    void unsubscribe(String dest, int connectionId); // Added

    void saveMessage(String destination, String user, String body); // Added
}
