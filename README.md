![semaforo](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTKbw-lMej4nVH1r6zZ3DhuXDCs3Xmyg3NfZQ&usqp=CAU)

# IC Baliza 


- para utilizar IC Baliza, primero crear el archivo de constantes. Dentro de la carpeta include, crear "secrets.h" y definir:
  - WIFI_SSID
  - WIFI_PASSWORD
  - CONFIG_EXAMPLE_BASIC_AUTH_USERNAME
  - CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD


Ejemplo de arcivo secrets.h:

```C
#ifndef ICBALIZA_SECRETS_H
#define ICBALIZA_SECRETS_H

#define WIFI_SSID "ssid_de_tu_wifi"
#define WIFI_PASSWORD "clave_super_secreta_de_wifi"
#define REPO_OWNER "Tuxe88"
#define REPO_NAME "ICBalizaTester"
#define CONFIG_EXAMPLE_BASIC_AUTH_USERNAME "admin"
#define CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD "password_admin"

#endif //ICBALIZA_SECRETS_H
```