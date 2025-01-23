package bgu.spl.net.api;

import java.util.LinkedHashMap;
import java.util.Map;


public class frame {

    private String command;                // e.g. "CONNECT", "SEND", "SUBSCRIBE", etc.
    private Map<String, String> headers;   // e.g. "receipt" -> "123", "destination" -> "/topic/foo"
    private String body;                   // The text body of the frame (may be empty)

    public frame() {
        this.headers = new LinkedHashMap<>();
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
        sb.append(body);
        return sb.toString();
    }

    public static frame fromString(String frameString) {
        // Split the first line for the command
        String[] parts = frameString.split("\n", 2);
        if (parts.length == 0) {
            // Malformed or empty
            return null;
        }
        String command = parts[0].trim();

        String headersAndMaybeBody = (parts.length > 1) ? parts[1] : "";
        // Split headers vs. body by "\n\n"
        String[] headerBodySplit = headersAndMaybeBody.split("\n\n", 2);
        String headerPart = headerBodySplit[0];
        String bodyPart = (headerBodySplit.length > 1) ? headerBodySplit[1] : "";

        Map<String, String> headers = new LinkedHashMap<>();
        if (!headerPart.isEmpty()) {
            // Each header is "key:value"
            for (String line : headerPart.split("\n")) {
                int idx = line.indexOf(':');
                if (idx > 0 && idx < line.length() - 1) {
                    String key = line.substring(0, idx).trim();
                    String val = line.substring(idx + 1).trim();
                    headers.put(key, val);
                }
            }
        }
        return new frame(command, headers, bodyPart);
    }
}
