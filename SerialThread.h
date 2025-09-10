//
// Created by Андрей Водолацкий on 09.09.2025.
//

#ifndef UNTITLED_SERIALTHREAD_H
#define UNTITLED_SERIALTHREAD_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <thread>
#include <atomic>



//  Потокобезопасная очередь
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Запрещаем копирование
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Добавление в очередь
    void enqueue(const T& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_cond.notify_one();
    }

    void enqueue(T&& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(data));
        m_cond.notify_one();
    }

    // Извлечение из очереди
    T dequeue() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this]{ return !m_queue.empty(); });
        T val = std::move(m_queue.front());
        m_queue.pop();
        return val;
    }

    // Попытка извлечения без блокировки
    bool try_dequeue(T& result) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        result = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // Попытка извлечения с таймаутом
    bool try_dequeue_with_timeout(T& result, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (!m_cond.wait_for(lock, timeout, [this]{ return !m_queue.empty(); })) {
            return false;
        }

        result = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // Проверка состояния
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    // Очистка очереди
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            m_queue.pop();
        }
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
};


//  Поток для работы с портом (POSIX API)
class SerialPort {
public:
    SerialPort(const char *device) : m_device(device), m_fd(-1), m_running(false) {}

    bool openPort() {
        m_fd = open(m_device, O_RDWR | O_NOCTTY | O_SYNC);
        if (m_fd < 0) {
            perror("open");
            return false;
        }

        struct termios tty;
        memset(&tty, 0, sizeof tty);
        if (tcgetattr(m_fd, &tty) != 0) {
            perror("tcgetattr");
            close(m_fd);
            return false;
        }

        cfsetospeed(&tty, B9600);
        cfsetispeed(&tty, B9600);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = 10; // 1 second read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
            perror("tcsetattr");
            close(m_fd);
            return false;
        }
        return true;
    }

    void closePort() {
        if (m_fd >= 0) close(m_fd);
        m_fd = -1;
    }

    int getFd() const { return m_fd; }

    ~SerialPort() { closePort(); }

private:
    const char *m_device;
    int m_fd;
    bool m_running;
};


// Поток обработки порта (чтение-запись)
class SerialThread {
public:
    SerialThread(const char *device,
                 ThreadSafeQueue<std::vector<uint8_t>> &outQueue,
                 ThreadSafeQueue<std::vector<uint8_t>> &inQueue)
        : m_port(device), m_outQueue(outQueue), m_inQueue(inQueue), m_stopFlag(false) {}

    void start() {
        if (!m_port.openPort()) return;
        m_thread = std::thread(&SerialThread::process, this);
    }

    void stop() {
        m_stopFlag.store(true);
        if (m_thread.joinable()) m_thread.join();
        m_port.closePort();
    }

private:
    void process() {
        int fd = m_port.getFd();
        uint8_t buffer[256];

        while (!m_stopFlag.load()) {
            fd_set readfds, writefds;
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_SET(fd, &readfds);
            FD_SET(fd, &writefds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100 ms

            int ret = select(fd + 1, &readfds, &writefds, NULL, &timeout);
            if (ret < 0) {
                perror("select");
                break;
            }

            // Чтение
            if (FD_ISSET(fd, &readfds)) {
                ssize_t n = read(fd, buffer, sizeof(buffer));
                if (n > 0) {
                    std::vector<uint8_t> data(buffer, buffer + n);
                    m_inQueue.enqueue(data);
                }
            }

            // Запись
            std::vector<uint8_t> dataToSend;
            if (m_outQueue.try_dequeue(dataToSend)) {
                ssize_t n = write(fd, dataToSend.data(), dataToSend.size());
                if (n < 0) {
                    perror("write");
                }
            }
        }
    }

    SerialPort m_port;
    std::thread m_thread;
    std::atomic<bool> m_stopFlag;
    ThreadSafeQueue<std::vector<uint8_t>> &m_outQueue;
    ThreadSafeQueue<std::vector<uint8_t>> &m_inQueue;
};

#endif //UNTITLED_SERIALTHREAD_H