From weaveworksdemos/carts:0.4.8
ENV CLIENT 139.224.206.51:3001
USER root
RUN apk update && apk add openjdk8-jre
COPY zookeeper-1.0-SNAPSHOT-jar-with-dependencies.jar zk.jar
CMD java -jar zk.jar  -n carts -h 172.20.240.5 -c $CLIENT; /usr/local/bin/java.sh app.jar --port=80
