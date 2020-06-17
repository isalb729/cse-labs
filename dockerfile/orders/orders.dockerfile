FROM weaveworksdemos/orders:0.4.7
USER root

RUN apk update \
     && apk add openjdk8-jre
ENV CLIENT 139.224.206.51:3001
COPY zookeeper-1.0-SNAPSHOT-jar-with-dependencies.jar zk.jar

ENTRYPOINT  java -jar zk.jar  -n orders -h 172.20.240.4 -c $CLIENT  ; java -jar ./app.jar --port=80
