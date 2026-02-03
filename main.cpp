/**
 * @file main.cpp
 * @brief 应用程序入口文件
 * @details 包含应用程序的主函数，负责初始化Qt应用并显示主窗口
 */

#include "mainwindow.h"
#include <QApplication>

/**
 * @brief 应用程序主函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 应用程序退出码
 * @details 初始化Qt应用程序对象，创建并显示主窗口，进入事件循环
 */
int main(int argc, char *argv[])
{
    // 创建Qt应用程序对象
    QApplication a(argc, argv);
    
    // 创建主窗口对象
    MainWindow w;
    
    // 显示主窗口
    w.show();
    
    // 进入Qt事件循环，等待用户交互
    return a.exec();
}
