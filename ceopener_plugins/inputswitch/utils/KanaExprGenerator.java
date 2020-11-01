import java.io.*;
import java.util.*;

public class KanaExprGenerator {
    public static void main(String[] args) throws Exception {
        BufferedReader br =
            new BufferedReader(new InputStreamReader(System.in));
        String line = br.readLine();

        String[] splited = line.split("\\s*,\\s*");
        for (String word : splited) {
            StringBuilder buff = new StringBuilder("{");
            int len = word.length();

            for (int i = 0; i < len; i++) {
                buff.append(String.format("0x%04x", (int)word.charAt(i)));
                buff.append(", ");
            }

            buff.append("0}");
            System.out.print(buff.toString() + ", ");
        }
    }
}

