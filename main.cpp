#include "ContactBook.h"
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
struct InputEnded {};

std::string ReadText(const std::string& prompt, bool allowEmpty = false) {
    for (;;) {
        std::cout << prompt;
        std::string value;
        if (!std::getline(std::cin, value)) throw InputEnded();
        if (allowEmpty || value.find_first_not_of(" \t\r") != std::string::npos)
            return value;
        std::cout << "输入不能为空。\n";
    }
}

int ReadInt(const std::string& prompt, int minimum = 1, int maximum = 2147483647) {
    for (;;) {
        std::string line = ReadText(prompt);
        std::istringstream input(line);
        int value;
        char extra;
        if ((input >> value) && !(input >> extra)
            && value >= minimum && value <= maximum) return value;
        std::cout << "请输入范围内的整数。\n";
    }
}

bool Confirm(const std::string& prompt) {
    const std::string answer = ReadText(prompt + " (y/N): ", true);
    return answer == "y" || answer == "Y";
}

void FillDemo(ContactBook& book) {
    ContactBook demo;
    demo.AddGroup(1, "同学");
    demo.AddGroup(2, "老师");
    demo.AddGroup(3, "家人");
    demo.AddContact(1, Contact(1002, "李四", "13900001002", "lisi@example.com"));
    demo.AddContact(1, Contact(1001, "张三", "13800001001", "zhangsan@example.com"));
    demo.AddContact(2, Contact(2001, "王老师", "13700002001", "wang@example.com"));
    demo.AddContact(3, Contact(3001, "妈妈", "13600003001", ""));
    book = demo; // 调用教师原版 LinkList 的深赋值运算符
}

void ShowMenu() {
    std::cout << "\n========== 分类通讯录管理系统 ==========\n"
              << " 1. 显示全部分组和联系人\n"
              << " 2. 添加分组         3. 删除分组\n"
              << " 4. 添加联系人       5. 修改联系人\n"
              << " 6. 删除联系人       7. 全局查找联系人\n"
              << " 8. 组内排序/倒置    9. 移动联系人\n"
              << "10. 统计人数        11. 保存到文件\n"
              << "12. 从文件读取      13. 加载演示数据\n"
              << " 0. 退出\n"
              << "=======================================\n";
}

Group* SelectGroup(ContactBook& book) {
    Group* g = book.FindGroup(ReadInt("分组编号: "));
    if (!g) std::cout << "分组不存在，请先添加分组。\n";
    return g;
}

void Run(ContactBook& book) {
    bool changed = false;
    for (;;) {
        ShowMenu();
        const int choice = ReadInt("请选择（0-13）: ", 0, 13);
        switch (choice) {
        case 0:
            if (changed && Confirm("有尚未保存的数据，退出前是否保存")) {
                const std::string path = ReadText("文件名（例如 addressbook.txt）: ");
                if (!book.Save(path)) {
                    std::cout << "保存失败，仍停留在菜单，请换一个路径重试。\n";
                    break;
                }
                std::cout << "保存成功。\n";
            }
            std::cout << "程序结束，自动释放两层链表。\n";
            return;
        case 1:
            if (book.Groups().NumNodes() == 0) std::cout << "通讯录为空。\n";
            else book.Groups().PutList(std::cout);
            break;
        case 2: {
            const int id = ReadInt("新分组编号: ");
            const std::string name = ReadText("分组名称: ");
            if (book.AddGroup(id, name)) {
                changed = true;
                std::cout << "分组添加成功。\n";
            } else std::cout << "添加失败，分组编号重复。\n";
            break;
        }
        case 3: {
            Group* g = SelectGroup(book);
            if (!g) break;
            std::cout << "将删除分组“" << g->name << "”及其 "
                      << g->contacts.NumNodes() << " 位联系人。\n";
            if (Confirm("确认删除")) {
                book.DeleteGroup(g->id);
                changed = true;
                std::cout << "分组及内层联系人链表已删除。\n";
            }
            break;
        }
        case 4: {
            Group* g = SelectGroup(book);
            if (!g) break;
            Contact c;
            c.id = ReadInt("联系人编号（整个通讯录内唯一）: ");
            if (book.HasContact(c.id)) {
                std::cout << "编号已存在。\n";
                break;
            }
            c.name = ReadText("姓名: ");
            c.phone = ReadText("电话: ");
            c.email = ReadText("邮箱（可直接回车留空）: ", true);
            if (book.AddContact(g->id, c)) {
                changed = true;
                std::cout << "联系人添加成功。\n";
            }
            break;
        }
        case 5: {
            Group* g = SelectGroup(book);
            if (!g) break;
            const int id = ReadInt("联系人编号: ");
            if (!g->contacts.Locate(id, true)) {
                std::cout << "本分组中不存在该联系人。\n";
                break;
            }
            std::cout << "原信息: " << g->contacts.CurData() << '\n';
            Contact c;
            c.id = id;
            c.name = ReadText("新姓名: ");
            c.phone = ReadText("新电话: ");
            c.email = ReadText("新邮箱（可留空）: ", true);
            book.UpdateContact(g->id, c);
            changed = true;
            std::cout << "修改成功，编号保持不变。\n";
            break;
        }
        case 6: {
            Group* g = SelectGroup(book);
            if (!g) break;
            const int id = ReadInt("联系人编号: ");
            if (!g->contacts.Locate(id, true)) {
                std::cout << "本分组中不存在该联系人。\n";
                break;
            }
            std::cout << g->contacts.CurData() << '\n';
            if (Confirm("确认删除联系人")) {
                book.DeleteContact(g->id, id);
                changed = true;
                std::cout << "删除成功。\n";
            }
            break;
        }
        case 7: {
            const std::string key = ReadText("编号/姓名/电话/邮箱关键字: ");
            const std::size_t found = book.Search(key, std::cout);
            std::cout << "共找到 " << found << " 位联系人。\n";
            break;
        }
        case 8: {
            Group* g = SelectGroup(book);
            if (!g) break;
            const int type = ReadInt("1 按编号，2 按姓名，3 倒置: ", 1, 3);
            if (type == 3) g->contacts.Reverse();
            else {
                const bool ascending = ReadInt("1 升序，2 降序: ", 1, 2) == 1;
                if (type == 1) g->contacts.Sort(int(0), ascending);
                else g->contacts.Sort(std::string(), ascending);
            }
            changed = true;
            std::cout << *g;
            break;
        }
        case 9: {
            const int from = ReadInt("原分组编号: ");
            const int to = ReadInt("目标分组编号: ");
            const int id = ReadInt("联系人编号: ");
            if (book.MoveContact(from, to, id)) {
                changed = true;
                std::cout << "移动成功。\n";
            } else std::cout << "移动失败：检查分组和联系人编号，两个分组必须不同。\n";
            break;
        }
        case 10:
            book.Statistics(std::cout);
            break;
        case 11: {
            const std::string path = ReadText("保存文件名（例如 addressbook.txt）: ");
            if (book.Save(path)) {
                changed = false;
                std::cout << "保存成功。\n";
            } else std::cout << "保存失败，请检查目录和文件权限。\n";
            break;
        }
        case 12:
            if (changed && !Confirm("读取将覆盖未保存的数据，是否继续")) break;
            if (book.Load(ReadText("读取文件名（例如 sample.txt）: "))) {
                changed = false;
                std::cout << "读取成功。\n";
            } else std::cout << "读取失败：文件不存在或格式无效；原数据保留。\n";
            break;
        case 13:
            if (book.Groups().NumNodes() && !Confirm("演示数据将覆盖当前数据，是否继续"))
                break;
            FillDemo(book);
            changed = true;
            std::cout << "已加载 3 个分组、4 位虚构联系人。\n";
            break;
        }
    }
}
} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    ContactBook book;
    try {
        Run(book);
    } catch (const InputEnded&) {
        std::cout << "\n输入结束，程序退出；未保存的修改不写入文件。\n";
    } catch (int code) {
        std::cerr << "链表操作失败，错误码: " << code << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "程序错误: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
