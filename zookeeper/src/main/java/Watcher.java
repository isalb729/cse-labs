import org.I0Itec.zkclient.IZkChildListener;
import org.I0Itec.zkclient.IZkDataListener;
import org.I0Itec.zkclient.IZkStateListener;
import org.I0Itec.zkclient.ZkClient;
import org.apache.zookeeper.Watcher.Event.KeeperState;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;

public class Watcher extends Thread {
    private static final int SECOND = 1000;
    private static ZkClient zk = null;
    private static String node;
    private final String client;
    private final String host;
    private final String name;

    public Watcher(String name, String host, String client) {
        this.name = name;
        this.host = host;
        this.client = client;
    }

    private void initialize() {
        zk = new ZkClient(client, 5 * SECOND);
        node = "/hosts";
        boolean exist = zk.exists(node);
        if (!exist) {
            zk.createPersistent(node, true);
        }
        zk.subscribeChildChanges(node, new ZkChildListener());
        zk.subscribeStateChanges(new ZkStateListener());
        zk.subscribeDataChanges(node, new ZkDataListener());
    }

    private void register() {
        boolean exist = zk.exists(node);
        if (exist) {
            HashMap<String, String> data = new HashMap<>();
            data.put(host, name);
            zk.writeData(node, data);
        }
    }

    private void writeHost(HashMap<String, String> data) throws IOException {
        File hosts = new File("/etc/hosts");
        if (!hosts.exists()) {
            hosts.createNewFile();
        }
        BufferedWriter bufferedWriter = new BufferedWriter(new FileWriter(hosts));
        data.forEach((k, v) -> {
            try {
                bufferedWriter.write(k + "  " + v + "\n");
            } catch (IOException e) {
                e.printStackTrace();
            }
        });
        bufferedWriter.flush();
    }

    private void fetch() throws IOException {
        boolean exist = zk.exists(node);
        if (exist) {
            HashMap<String, String> hashMap = zk.readData(node);
            hashMap.put(host, name);
            writeHost(hashMap);
        }
    }

    private void monitor() {
        while (true) {
            try {
                sleep(2 * SECOND);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void run() {
        super.run();
        initialize();
        try {
            fetch();
        } catch (IOException e) {
            e.printStackTrace();
        }
        monitor();
    }

    static class ZkStateListener implements IZkStateListener {
        public void handleStateChanged(KeeperState state) throws Exception {
        }

        public void handleNewSession() throws Exception {
        }
    }

    class ZkDataListener implements IZkDataListener {
        public void handleDataChange(String dataPath, Object data) throws Exception {
            writeHost((HashMap<String, String>) data);
        }

        public void handleDataDeleted(String dataPath) throws Exception {
        }
    }

    static class ZkChildListener implements IZkChildListener {
        public void handleChildChange(String parentPath, List<String> currentChildren) throws Exception {
        }
    }
}
