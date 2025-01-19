package bgu.spl.net.srv;

import java.util.HashMap;
import java.util.Map;

public class frame {

    private String command;                // e.g. "CONNECT", "SEND", "SUBSCRIBE", etc.
    private Map<String, String> headers;   // e.g. "receipt" -> "123", "destination" -> "/topic/foo"
    private String body;                   // The text body of the frame (may be empty)

    public frame() {
        this.headers = new HashMap<>();
        this.body = "";
    }

    public frame(String command, Map<String, String> headers, String body) {
        this.command = command;
        this.headers = headers;
        this.body = body;
    }

    public String getCommand() {
        return command;
    }

    public void setCommand(String command) {
        this.command = command;
    }

    public Map<String, String> getHeaders() {
        return headers;
    }

    public void setHeaders(Map<String, String> headers) {
        this.headers = headers;
    }

    public String getHeader(String key) {
        return headers.get(key);
    }

    public void addHeader(String key, String value) {
        this.headers.put(key, value);
    }

    public String getBody() {
        return body;
    }

    public void setBody(String body) {
        this.body = body;
    }

    /**
     * Returns a String in the standard STOMP format:
     * COMMAND
     * header1:value1
     * header2:value2
     *
     * body
     * \0
     */
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(command).append("\n");
        for (Map.Entry<String, String> entry : headers.entrySet()) {
            sb.append(entry.getKey()).append(":").append(entry.getValue()).append("\n");
        }
        sb.append("\n");            // Blank line separating headers and body
        sb.append(body).append("\0"); // Null terminator at the end
        return sb.toString();
    }
}
