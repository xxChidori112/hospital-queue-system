#include "hospital_system.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
static constexpr Socket InvalidSocket = INVALID_SOCKET;
static void closeSocket(Socket socket) { closesocket(socket); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
static constexpr Socket InvalidSocket = -1;
static void closeSocket(Socket socket) { close(socket); }
#endif

namespace {
unsigned short serverPort() {
    const char* value = std::getenv("PORT");
    if (value == nullptr || *value == '\0') return 8080;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 1 || parsed > 65535)
        throw std::runtime_error("PORT phai la so trong khoang 1-65535.");
    return static_cast<unsigned short>(parsed);
}

struct Request {
    std::string method;
    std::string path;
    std::string body;
};

std::string jsonEscape(const std::string& input) {
    std::ostringstream out;
    for (const unsigned char c : input) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789abcdef";
                    out << "\\u00" << hex[c >> 4] << hex[c & 0x0f];
                } else out << static_cast<char>(c);
        }
    }
    return out.str();
}

std::string urlDecode(const std::string& value) {
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+' ) result.push_back(' ');
        else if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long decoded = std::strtol(hex.c_str(), &end, 16);
            if (end != hex.c_str() && *end == '\0') {
                result.push_back(static_cast<char>(decoded));
                i += 2;
            } else result.push_back(value[i]);
        } else result.push_back(value[i]);
    }
    return result;
}

std::map<std::string, std::string> parseForm(const std::string& body) {
    std::map<std::string, std::string> values;
    std::size_t start = 0;
    while (start < body.size()) {
        const auto end = body.find('&', start);
        const auto pair = body.substr(start, end == std::string::npos ? end : end - start);
        const auto equals = pair.find('=');
        values[urlDecode(pair.substr(0, equals))] =
            equals == std::string::npos ? "" : urlDecode(pair.substr(equals + 1));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return values;
}

std::string field(const std::map<std::string, std::string>& form, const std::string& key) {
    const auto it = form.find(key);
    return it == form.end() ? "" : it->second;
}

std::string patientJson(const Patient& patient, const HospitalSystem& system) {
    std::ostringstream out;
    out << "{\"ticket\":\"" << jsonEscape(patient.ticket)
        << "\",\"name\":\"" << jsonEscape(patient.name)
        << "\",\"age\":" << patient.age
        << ",\"birthYear\":" << patient.birthYear
        << ",\"gender\":\"" << jsonEscape(patient.gender)
        << "\",\"category\":\"" << jsonEscape(patient.category) << "\""
        << ",\"symptom\":\"" << jsonEscape(patient.symptom)
        << "\",\"department\":\"" << jsonEscape(patient.department)
        << "\",\"triageLevel\":" << patient.triageLevel
        << ",\"arrivalTime\":\"" << jsonEscape(patient.arrivalTime)
        << "\",\"status\":\"" << jsonEscape(HospitalSystem::statusText(patient.status))
        << "\",\"emergency\":" << (patient.emergency ? "true" : "false")
        << ",\"waitMinutes\":";
    const auto wait = system.estimatedWaitMinutes(patient.ticket);
    if (wait) out << *wait;
    else out << "null";
    out << '}';
    return out.str();
}

std::string stateJson(const HospitalSystem& system) {
    std::ostringstream out;
    out << "{\"patients\":[";
    const auto patients = system.allPatients();
    for (std::size_t i = 0; i < patients.size(); ++i) {
        if (i) out << ',';
        out << patientJson(patients[i], system);
    }
    out << "],\"waiting\":[";
    const auto waiting = system.waitingPatients();
    for (std::size_t i = 0; i < waiting.size(); ++i) {
        if (i) out << ',';
        out << patientJson(waiting[i], system);
    }
    out << "],\"history\":[";
    const auto history = system.callHistory();
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (i) out << ',';
        const auto& record = history[i];
        out << "{\"ticket\":\"" << jsonEscape(record.ticket)
            << "\",\"name\":\"" << jsonEscape(record.patientName)
            << "\",\"department\":\"" << jsonEscape(record.department)
            << "\",\"calledAt\":\"" << jsonEscape(record.calledAt)
            << "\",\"emergency\":" << (record.emergency ? "true" : "false") << '}';
    }
    out << "],\"departments\":[";
    const auto& departments = system.departments();
    for (std::size_t i = 0; i < departments.size(); ++i) {
        if (i) out << ',';
        out << '"' << jsonEscape(departments[i]) << '"';
    }
    out << "],\"waitingCount\":" << system.waitingCount() << '}';
    return out.str();
}

Request readRequest(Socket client) {
    std::string raw;
    char buffer[4096];
    std::size_t headerEnd = std::string::npos;
    std::size_t contentLength = 0;
    for (;;) {
        const int received = recv(client, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (received <= 0) break;
        raw.append(buffer, static_cast<std::size_t>(received));
        if (headerEnd == std::string::npos) {
            headerEnd = raw.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string headers = raw.substr(0, headerEnd);
                std::transform(headers.begin(), headers.end(), headers.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                const auto lengthPos = headers.find("content-length:");
                if (lengthPos != std::string::npos) {
                    const auto valueStart = headers.find_first_not_of(" \t", lengthPos + 15);
                    if (valueStart != std::string::npos) contentLength = static_cast<std::size_t>(std::strtoul(headers.c_str() + valueStart, nullptr, 10));
                }
            }
        }
        if (headerEnd != std::string::npos && raw.size() >= headerEnd + 4 + contentLength) break;
        if (raw.size() > 1024 * 1024) break;
    }
    if (headerEnd == std::string::npos) throw std::runtime_error("Yeu cau HTTP khong hop le.");

    Request request;
    std::istringstream firstLine(raw.substr(0, raw.find("\r\n")));
    firstLine >> request.method >> request.path;
    request.body = raw.substr(headerEnd + 4, contentLength);
    return request;
}

void sendResponse(Socket client, int status, const std::string& contentType, const std::string& body) {
    const std::string statusText = status == 200 ? "OK" : status == 400 ? "Bad Request" : status == 404 ? "Not Found" : "Internal Server Error";
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << statusText << "\r\n"
             << "Content-Type: " << contentType << "; charset=utf-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n" << body;
    const std::string data = response.str();
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int count = send(client, data.data() + sent, static_cast<int>(data.size() - sent), 0);
        if (count <= 0) break;
        sent += static_cast<std::size_t>(count);
    }
}

void handleRequest(Socket client, HospitalSystem& system) {
    try {
        const Request request = readRequest(client);
        if (request.method == "GET" && (request.path == "/" || request.path == "/index.html")) {
            std::ifstream file("web/index.html", std::ios::binary);
            if (!file) { sendResponse(client, 500, "text/plain", "Khong mo duoc web/index.html. Hay chay tu thu muc du an."); return; }
            std::ostringstream html;
            html << file.rdbuf();
            sendResponse(client, 200, "text/html", html.str());
            return;
        }
        if (request.method == "GET" && request.path == "/app.js") {
            std::ifstream file("web/app.js", std::ios::binary);
            if (!file) { sendResponse(client, 404, "text/plain", "Khong tim thay app.js."); return; }
            std::ostringstream script;
            script << file.rdbuf();
            sendResponse(client, 200, "application/javascript; charset=utf-8", script.str());
            return;
        }
        if (request.method == "GET" && request.path == "/api/state") {
            sendResponse(client, 200, "application/json", stateJson(system));
            return;
        }
        if (request.method != "POST") { sendResponse(client, 404, "application/json", "{\"error\":\"Khong tim thay endpoint.\"}"); return; }

        const auto form = parseForm(request.body);
        if (request.path == "/api/register") {
            const auto birthYearText = field(form, "birthYear");
            const int birthYear = birthYearText.empty() ? 0 : std::stoi(birthYearText);
            const auto timestamp = HospitalSystem::currentTime();
            const int age = birthYear ? std::stoi(timestamp.substr(timestamp.size() - 4)) - birthYear : std::stoi(field(form, "age"));
            const int triage = std::stoi(field(form, "triage"));
            auto patient = system.registerPatient(field(form, "name"), age, field(form, "symptom"),
                                                  triage, field(form, "department"), birthYear,
                                                  field(form, "gender"), field(form, "category"));
            sendResponse(client, 200, "application/json", "{\"ok\":true,\"message\":\"Da tiep nhan " +
                         jsonEscape(patient.ticket) + "\"}");
        } else if (request.path == "/api/call-next") {
            const auto patient = system.callNextPatient(field(form, "department"));
            if (!patient) { sendResponse(client, 200, "application/json", "{\"ok\":false,\"message\":\"Khong co benh nhan dang cho.\"}"); return; }
            sendResponse(client, 200, "application/json", "{\"ok\":true,\"message\":\"Moi " +
                         jsonEscape(patient->name) + " (" + jsonEscape(patient->ticket) + ") den khoa " +
                         jsonEscape(patient->department) + "\",\"ticket\":\"" + jsonEscape(patient->ticket) + "\"}");
        } else if (request.path == "/api/call-specific") {
            const auto patient = system.callSpecificPatient(field(form, "ticket"));
            if (!patient) { sendResponse(client, 200, "application/json", "{\"ok\":false,\"message\":\"Khong tim thay benh nhan dang cho.\"}"); return; }
            sendResponse(client, 200, "application/json", "{\"ok\":true,\"message\":\"Da goi benh nhan.\",\"ticket\":\"" + jsonEscape(patient->ticket) + "\"}");
        } else if (request.path == "/api/complete") {
            const bool ok = system.completeConsultation(field(form, "ticket"));
            sendResponse(client, 200, "application/json", ok ? "{\"ok\":true,\"message\":\"Da hoan tat luot kham.\"}" :
                         "{\"ok\":false,\"message\":\"Khong tim thay benh nhan dang kham.\"}");
        } else if (request.path == "/api/absent" || request.path == "/api/cancel" || request.path == "/api/transfer-department") {
            const std::string ticket = field(form, "ticket");
            bool ok = false;
            if (request.path == "/api/absent") ok = system.markAbsent(ticket);
            else if (request.path == "/api/cancel") ok = system.cancelPatient(ticket);
            else ok = system.transferDepartment(ticket, field(form, "department"));
            sendResponse(client, 200, "application/json", ok ? "{\"ok\":true,\"message\":\"Da cap nhat ho so.\"}" :
                         "{\"ok\":false,\"message\":\"Khong the cap nhat. Kiem tra trang thai va khoa.\"}");
        } else if (request.path == "/api/transfer") {
            const int level = std::stoi(field(form, "triage"));
            const bool ok = system.transferToEmergency(field(form, "ticket"), level);
            sendResponse(client, 200, "application/json", ok ? "{\"ok\":true,\"message\":\"Da chuyen vao hang uu tien cap cuu.\"}" :
                         "{\"ok\":false,\"message\":\"Khong the chuyen. Kiem tra ma phieu va trang thai cho.\"}");
        } else sendResponse(client, 404, "application/json", "{\"error\":\"Khong tim thay endpoint.\"}");
    } catch (const std::exception& error) {
        sendResponse(client, 400, "application/json", "{\"ok\":false,\"message\":\"" + jsonEscape(error.what()) + "\"}");
    }
}
}

int main() {
    unsigned short port = 0;
    try { port = serverPort(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
#ifdef _WIN32
    WSADATA winsockData{};
    if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0) {
        std::cerr << "Khong khoi dong duoc Winsock.\n";
        return 1;
    }
#endif
    Socket server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == InvalidSocket) {
        std::cerr << "Khong tao duoc socket.\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    const char* bindHost = std::getenv("HOST");
    if (bindHost == nullptr || *bindHost == '\0') bindHost = "127.0.0.1";
    if (inet_pton(AF_INET, bindHost, &address.sin_addr) != 1) {
        std::cerr << "HOST phai la dia chi IPv4 hop le.\n";
        closeSocket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || listen(server, 16) != 0) {
        std::cerr << "Khong mo duoc cong 8080. Co the cong dang duoc su dung.\n";
        closeSocket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    const char* databasePath = std::getenv("HOSPITAL_DB");
    HospitalSystem system(databasePath == nullptr || *databasePath == '\0' ? "hospital.db" : databasePath);
    std::cout << "Web app dang chay tren cong " << port << " (HOST=" << bindHost << ")\n"
              << "Nhan Ctrl+C de dung. Hay chay chuong trinh tu thu muc du an.\n";
    for (;;) {
        Socket client = accept(server, nullptr, nullptr);
        if (client == InvalidSocket) continue;
        handleRequest(client, system);
        closeSocket(client);
    }
}
