FROM weaveworksdemos/user:0.4.4

USER root

RUN apk update \
     && apk add openjdk8-jre
ENV CLIENT 139.224.206.51:3001
COPY zookeeper-1.0-SNAPSHOT-jar-with-dependencies.jar zk.jar
CMD  java -jar zk.jar -n user -h 172.20.240.3 -c $CLIENT;/user -port=80
