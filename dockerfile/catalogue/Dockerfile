FROM weaveworksdemos/catalogue:0.3.5

USER root

RUN apk update \
     && apk add openjdk8-jre
ENV CLIENT 139.224.206.51:3001
COPY zookeeper-1.0-SNAPSHOT-jar-with-dependencies.jar zk.jar
CMD java -jar zk.jar  -n catalogue -h 172.20.240.7 -c $CLIENT; /app -port=80


