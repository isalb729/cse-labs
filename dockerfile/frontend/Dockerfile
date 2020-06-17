FROM dplsming/sockshop-frontend:0.1

USER root

RUN apk update \
     && apk add openjdk8-jre
COPY *.jar app.jar
ENV CLIENT 139.224.206.51:3001
# frontend doesn't need a name
CMD  java -jar app.jar -n 0 -h 0 -c $CLIENT  ; /usr/local/bin/npm start  
