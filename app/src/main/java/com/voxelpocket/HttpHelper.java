package com.voxelpocket;

import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;

/**
 * HttpHelper — called from C++ JNI to make HTTP requests.
 * All methods are static; C++ calls them via FindClass/GetStaticMethodID.
 */
public class HttpHelper {

    /**
     * Perform an HTTP request and return the response body as a String.
     * @param method   GET, POST, PATCH, DELETE
     * @param url      Full URL
     * @param body     Request body (may be empty)
     * @param token    Firebase ID token (added as Authorization Bearer header if non-empty)
     * @return Response body string, or empty string on error
     */
    public static String request(String method, String url, String body, String token) {
        try {
            URL u = new URL(url);
            HttpURLConnection conn = (HttpURLConnection) u.openConnection();
            conn.setRequestMethod(method.equals("PATCH") ? "POST" : method);
            if (method.equals("PATCH")) {
                conn.setRequestProperty("X-HTTP-Method-Override", "PATCH");
            }
            conn.setRequestProperty("Content-Type", "application/json; charset=UTF-8");
            conn.setRequestProperty("Accept", "application/json");
            if (token != null && !token.isEmpty()) {
                conn.setRequestProperty("Authorization", "Bearer " + token);
            }
            conn.setConnectTimeout(8000);
            conn.setReadTimeout(8000);

            if (!body.isEmpty()) {
                conn.setDoOutput(true);
                byte[] bytes = body.getBytes(StandardCharsets.UTF_8);
                conn.setRequestProperty("Content-Length", String.valueOf(bytes.length));
                try (OutputStream os = conn.getOutputStream()) {
                    os.write(bytes);
                }
            }

            int code = conn.getResponseCode();
            InputStream is = (code >= 200 && code < 300)
                ? conn.getInputStream()
                : conn.getErrorStream();

            if (is == null) return "";
            StringBuilder sb = new StringBuilder();
            try (BufferedReader br = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8))) {
                String line;
                while ((line = br.readLine()) != null) sb.append(line);
            }
            return sb.toString();

        } catch (Exception e) {
            return "";
        }
    }
}
