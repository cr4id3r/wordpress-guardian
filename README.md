# Defensor de Archivos

Este proyecto es un sistema de monitoreo de archivos que notifica a través de Telegram cuando se realizan cambios en los archivos de ciertos directorios.

## Características

- Monitorea múltiples directorios de forma recursiva.
- Notifica a través de Telegram cuando se realizan cambios en los archivos.
- Agrupa las notificaciones y las envía cada 5 minutos.
- Ignora ciertos archivos temporales y directorios específicos.

## Dependencias

- C++17
- libcurl
- libinotify

## Uso

1. Clona el repositorio.
2. Compila el proyecto con un compilador compatible con C++17.
3. Ejecuta el binario resultante.

## Configuración

Puedes configurar los directorios a monitorear y los detalles del bot de Telegram en el archivo `main.cpp`.

```c++
const std::string FOLDERS_TO_WATCH[] = {
        "/var/www/demo.es",
        "/var/www/example.com"
};
const std::string TELEGRAM_BOT_TOKEN = "123456:asdfsdaf-fgewrfwefwefwef";
const std::string TELEGRAM_CHAT_ID = "34534123";
```

## Limitaciones

- Solo funciona en sistemas Linux que soportan inotify.
- No maneja errores de red o de la API de Telegram.

