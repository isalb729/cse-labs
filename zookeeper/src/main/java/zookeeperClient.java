import org.apache.commons.cli.*;
public class zookeeperClient {
    public static void main(String[] args) throws ParseException {
        Options options = new Options();
        options.addOption("h", true, "ip:port");
        options.addOption("n", true, "name");
        options.addOption("c", true, "client address");
        CommandLineParser parser = new DefaultParser();
        CommandLine cml = parser.parse(options, args);
        if (!cml.hasOption("h") || !cml.hasOption("n") || !cml.hasOption("c")) {
            return;
        }
        Watcher watcher = new Watcher(cml.getOptionValue("n"), cml.getOptionValue("h"),
                cml.getOptionValue("c"));
        watcher.start();
    }
}
