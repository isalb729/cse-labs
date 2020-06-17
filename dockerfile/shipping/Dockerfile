FROM weaveworksdemos/shipping:0.4.8
ENV CLIENT 139.224.206.51:3001
USER root
RUN apk update && apk add openjdk8-jre
COPY zookeeper-1.0-SNAPSHOT-jar-with-dependencies.jar zk.jar
ENTRYPOINT java -jar zk.jar -n shipping -h 172.20.240.2 -c $CLIENT;  /usr/local/bin/java.sh -jar ./app.jar --port=80
