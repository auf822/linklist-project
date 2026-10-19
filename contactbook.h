#ifndef CONTACTBOOK_H
#define CONTACTBOOK_H

#include "LinkList.h"
#include <iomanip>
#include <string>
#include <utility>

// 联系人类：参考 AddressBook.cpp 的字段和类型转换运算符。
struct Contact {
    int id = 0;
    std::string name, phone, email;
    Contact() = default;
    Contact(int i, std::string n, std::string p, std::string e)
        : id(i), name(std::move(n)), phone(std::move(p)), email(std::move(e)) {}
    operator int() const { return id; }
    operator std::string() const { return name; }
};

inline std::ostream& operator<<(std::ostream& out, const Contact& c) {
    return out << "编号: " << c.id << " | 姓名: " << c.name
               << " | 电话: " << c.phone << " | 邮箱: " << c.email;
}

struct Group {
    int id = 0;
    std::string name;
    LinkList<Contact> contacts;       // 内层链表：本分组的联系人
    Group() = default;
    Group(int i, std::string n) : id(i), name(std::move(n)) {}
    operator int() const { return id; }
    operator std::string() const { return name; }
};

inline std::ostream& operator<<(std::ostream& out, const Group& g) {
    out << "\n[分组 " << g.id << "] " << g.name
        << "（" << g.contacts.NumNodes() << " 人）\n";
    if (g.contacts.NumNodes() == 0) out << "  暂无联系人\n";
    else g.contacts.PutList(out);
    return out;
}

class ContactBook {
private:
    LinkList<Group> groups_;          // 外层链表：所有分组

public:
    const LinkList<Group>& Groups() const { return groups_; }

    Group* FindGroup(int id) {
        if (groups_.Locate(id, true) == NULL) return nullptr;
        return &groups_.CurData();
    }

    // 原版 Node 的 data/next 是私有的，必须通过 LinkList 的公有接口遍历。
    // 遍历会改变链表的“当前结点”，因此这些成员函数不声明为 const。
    bool HasContact(int id) {
        for (groups_.GoTop(); groups_.CurNode() != NULL; groups_.Skip()) {
            LinkList<Contact>& contacts = groups_.CurData().contacts;
            for (contacts.GoTop(); contacts.CurNode() != NULL; contacts.Skip())
                if (contacts.CurData().id == id) return true;
        }
        return false;
    }

    bool AddGroup(int id, const std::string& name) {
        if (id <= 0 || name.empty() || FindGroup(id)) return false;
        groups_.Append(Group(id, name));
        return true;
    }

    bool DeleteGroup(int id) {
        if (!FindGroup(id)) return false;
        // 原版返回被删数据（这里为 Group），临时对象随后也会自动析构。
        groups_.DeleteCurNode();
        return true;
    }

    bool AddContact(int groupId, const Contact& c) {
        Group* g = FindGroup(groupId);
        if (!g || c.id <= 0 || c.name.empty() || c.phone.empty() || HasContact(c.id))
            return false;
        // HasContact 改变了游标，但不删除结点，前面得到的 g 仍然有效。
        g->contacts.Append(c);
        return true;
    }

    bool UpdateContact(int groupId, const Contact& c) {
        Group* g = FindGroup(groupId);
        if (!g || c.name.empty() || c.phone.empty() || !g->contacts.Locate(c.id, true))
            return false;
        g->contacts.ModifyData(c);     // 使用原版的修改接口
        return true;
    }

    bool DeleteContact(int groupId, int id) {
        Group* g = FindGroup(groupId);
        if (!g || !g->contacts.Locate(id, true)) return false;
        g->contacts.DeleteCurNode();  // 使用原版的删除接口
        return true;
    }

    bool MoveContact(int sourceId, int targetId, int contactId) {
        if (sourceId == targetId) return false;
        Group* source = FindGroup(sourceId);
        Group* target = FindGroup(targetId);
        if (!source || !target || !source->contacts.Locate(contactId, true))
            return false;
        target->contacts.Append(source->contacts.CurData());
        source->contacts.DeleteCurNode();
        return true;
    }

    std::size_t ContactCount() {
        std::size_t total = 0;
        for (groups_.GoTop(); groups_.CurNode() != NULL; groups_.Skip())
            total += groups_.CurData().contacts.NumNodes();
        return total;
    }

    void Statistics(std::ostream& out) {
        for (groups_.GoTop(); groups_.CurNode() != NULL; groups_.Skip()) {
            const Group& g = groups_.CurData();
            out << "[" << g.id << "] " << g.name << ": "
                << g.contacts.NumNodes() << " 人\n";
        }
        out << "共 " << groups_.NumNodes() << " 个分组，"
            << ContactCount() << " 位联系人。\n";
    }

    std::size_t Search(const std::string& keyword, std::ostream& out) {
        std::size_t count = 0;
        // 两重循环体现“链表的链表”：先遍历分组，再遍历本组联系人。
        for (groups_.GoTop(); groups_.CurNode() != NULL; groups_.Skip()) {
            Group& g = groups_.CurData();
            for (g.contacts.GoTop(); g.contacts.CurNode() != NULL; g.contacts.Skip()) {
                const Contact& c = g.contacts.CurData();
                if (std::to_string(c.id) == keyword
                    || c.name.find(keyword) != std::string::npos
                    || c.phone.find(keyword) != std::string::npos
                    || c.email.find(keyword) != std::string::npos) {
                    out << "[" << g.name << "] " << c << '\n';
                    ++count;
                }
            }
        }
        return count;
    }

    // 按“分组记录 + 该组联系人记录”保存，沿用上一版的数据文件格式。
    // 文件读写在应用层实现；原版 LinkList 的 Save/Load、GetList 未改动。
    bool Save(const std::string& filename) {
        std::ofstream out(filename);
        if (!out) return false;
        out << "GROUPED_ADDRESS_BOOK_V1\n" << groups_.NumNodes() << '\n';
        for (groups_.GoTop(); groups_.CurNode() != NULL; groups_.Skip()) {
            Group& g = groups_.CurData();
            out << "G " << g.id << ' ' << std::quoted(g.name)
                << ' ' << g.contacts.NumNodes() << '\n';
            for (g.contacts.GoTop(); g.contacts.CurNode() != NULL; g.contacts.Skip()) {
                const Contact& c = g.contacts.CurData();
                out << "C " << c.id << ' ' << std::quoted(c.name) << ' '
                    << std::quoted(c.phone) << ' ' << std::quoted(c.email) << '\n';
            }
        }
        out.close();
        return static_cast<bool>(out);
    }

    bool Load(const std::string& filename) {
        std::ifstream in(filename);
        std::string header;
        long long groupCount = 0;
        if (!in || !std::getline(in, header)) return false;
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header != "GROUPED_ADDRESS_BOOK_V1"
            || !(in >> groupCount) || groupCount < 0 || groupCount > 10000)
            return false;
        ContactBook temp;
        for (long long i = 0; i < groupCount; ++i) {
            char tag;
            int groupId;
            std::string groupName;
            long long contactCount;
            if (!(in >> tag >> groupId >> std::quoted(groupName) >> contactCount)
                || tag != 'G' || contactCount < 0 || contactCount > 100000
                || !temp.AddGroup(groupId, groupName)) return false;
            for (long long j = 0; j < contactCount; ++j) {
                Contact c;
                if (!(in >> tag >> c.id >> std::quoted(c.name)
                         >> std::quoted(c.phone) >> std::quoted(c.email))
                    || tag != 'C' || !temp.AddContact(groupId, c)) return false;
            }
        }
        in >> std::ws;
        if (!in.eof()) return false;
        // 原版没有 Swap()，直接使用原版已经实现的深赋值。
        groups_ = temp.groups_;
        return true;
    }
};

#endif
