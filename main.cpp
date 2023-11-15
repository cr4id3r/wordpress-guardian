#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/inotify.h>
#include <curl/curl.h>
#include <unordered_map>
#include <chrono>
#include <filesystem>

#include <queue>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

const int BUF_LEN = (10 * (sizeof(struct inotify_event) + NAME_MAX + 1));
const std::string FOLDERS_TO_WATCH[] = {
        "/var/www/example.es",
        "/var/www/demo.com"
};
const std::string TELEGRAM_BOT_TOKEN = "telegram_bot_token";
const std::string TELEGRAM_CHAT_ID = "telgram_chat_id";

// Create a variable to store the last modified file name and time to set max 1 notification every 5 minutes.
std::unordered_map<std::string, std::chrono::time_point<std::chrono::steady_clock>> lastNotified;
std::unordered_map<int, std::string> watchDescriptorToDirPath;

std::queue<std::string> messageQueue;
std::mutex mtx;

void send_telegram_message(const std::string& message) {
    mtx.lock();
    messageQueue.push(message);
    mtx.unlock();
}

void send_queued_messages() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(5));

        mtx.lock();
        std::string all_messages;
        while (!messageQueue.empty()) {
            std::string message = messageQueue.front();
            messageQueue.pop();
            all_messages += message + "\n"; // Concatena los mensajes con un salto de línea entre ellos
        }

        if (!all_messages.empty()) {
            CURL *curl;
            CURLcode res;

            std::string url = "https://api.telegram.org/bot" + TELEGRAM_BOT_TOKEN + "/sendMessage";
            std::stringstream post_fields_stream;
            post_fields_stream << "chat_id=" << TELEGRAM_CHAT_ID << "&text=" << all_messages;
            std::string post_fields = post_fields_stream.str();

            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl = curl_easy_init();
            if(curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

                // Realizar la solicitud
                res = curl_easy_perform(curl);

                // Limpiar
                curl_easy_cleanup(curl);
            }
            curl_global_cleanup();
        }
        mtx.unlock();
    }
}

int watch_directory_recursively(int inotify_instance, const fs::path& path) {
    int watch_descriptor = inotify_add_watch(inotify_instance, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO);
    std::cout << "Añadiendo carpeta " << path << " a inotify" << std::endl;

    if (watch_descriptor < 0) {
        std::cerr << "Error al añadir la carpeta a inotify" << std::endl;
        return -1;
    } else {
        std::cout << "Añadida carpeta " << path << " a inotify" << std::endl;
    }

    watchDescriptorToDirPath[watch_descriptor] = path;

    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            watch_directory_recursively(inotify_instance, entry.path());
        }
    }

    return watch_descriptor;
}

int main() {
    std::thread messageThread(send_queued_messages);

    int inotifyFd = inotify_init();

    std::cout << "Iniciando defensor.." << std::endl;

    if (inotifyFd < 0) {
        std::cerr << "Error al inicializar inotify" << std::endl;
        return 1;
    }

    for (const auto& folder : FOLDERS_TO_WATCH) {
        watch_directory_recursively(inotifyFd, folder);
    }

    char buffer[BUF_LEN];
    struct inotify_event *event;

    std::cout << "Defendiendo..." << std::endl;
    while (true) {
        ssize_t length = read(inotifyFd, buffer, BUF_LEN);

        if (length < 0) {
            std::cerr << "Error de lectura." << std::endl;
            return 1;
        }

        for (char *ptr = buffer; ptr < buffer + length; ptr += sizeof(struct inotify_event) + event->len) {
            event = (struct inotify_event *) ptr;

            if (event->mask) {
                std::cout << "Se ha modificado un archivo" << std::endl;

                // Ignora archivos temporales
                if (event->name[0] == '.') {
                    continue;
                }

                std::string fileName(event->name);

                if (fileName.rfind("~", fileName.size() - 1) != std::string::npos) {
                    continue;
                }

                std::cout << "Archivo modificado: " << event->name << std::endl;

                // Get full path of notified file
                std::string dirPath = watchDescriptorToDirPath[event->wd];
                std::string fullPath = dirPath + "/" + fileName;

                if (fullPath.find("temp-write-test") != std::string::npos) {
                    continue;
                }

                if (fullPath.find("wflogs") != std::string::npos) {
                    continue;
                }

                // Guarda el nombre del archivo en la memoria para evitar múltiples llamadas al endpoint.
                send_telegram_message("Se ha modificado el archivo\n " + std::string(fullPath));
            }
        }
    }

    messageThread.join();
    return 0;
}
