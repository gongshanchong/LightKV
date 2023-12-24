#ifndef IO_H
#define IO_H
#include <cstdio>
#include <iostream>

class KVio {
public:
    // 初始化成员变量并打开文件
    KVio(std::string fileName) : fileName_(fileName){
        fp_ = fopen(fileName_.data(), "r");
        if (!fp_) {
            printf("rdb file doesn't exits.");
            return;
        }
    }

    // 析构函数
    ~KVio() {
        flushCache();
        fclose(fp_);
    }

    // 向文件写入数组中的内容，返回写后剩余长度
    size_t kvioFileWrite(const void* buf, std::size_t len) {
        size_t retval;
        while (len) {
            retval = fwrite(buf, 1, len, fp_);
            len -= retval;
            buf = (char*)buf + retval;
        }
        return len;
    }

    // 从文件中读取特定长度的内容到数组
    size_t kvioFileRead(void* buf, std::size_t len) {
        size_t retval;
        while (len) {
            retval = fread(buf, 1, len, fp_);
            if (retval < 0) return -1;
            len -= retval;
            buf = (char*)buf + retval;
        }
        return len;
    }

    // 检测流上的文件结束符的函数，如果文件结束，则返回非0值，否则返回0
    bool reachEOF() {
        return feof(fp_);
    }

    // 判断文件是否为空
    bool empty() {
        if (!fp_) return true;
        // 判断是否到EOF
        getc(fp_);
        if (feof(fp_)) {
            return true; // empty file
        }
        fseek(fp_, -1, SEEK_CUR);
        return false;
    }

    // "w+" will clear the file or create the new file
    void clearFile() {
        fp_ = fopen(fileName_.data(), "w+"); 
    }

    // 更新缓存区
    inline void flushCache() {
        fflush(fp_);
    }

    // 重置到文件的开头
    inline void fseekTop() {
        fseek(fp_, 0, SEEK_SET);
    }

    // 文件句柄
    FILE* fp_;
    // 文件名
    std::string fileName_;
};

#endif