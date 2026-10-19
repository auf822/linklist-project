#ifndef CONTACTBOOK_H
#define CONTACTBOOK_H
#include "LinkList.h"
#include <fstream>
#include <iomanip>
#include <string>

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
    LinkList<Contact> contacts;       // 内层链表：该分组的联系人
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

// 管理类：把两层链表组织起来，界面在 main.cpp 中。
class ContactBook {
private:
    LinkList<Group> groups_;          // 外层链表：所有分组

public:
    const LinkList<Group>& Groups() const { return groups_; }

    Group* FindGroup(int id) {
        Node<Group>* p = groups_.Locate(id, true);
        return p ? &groups_.CurData() : nullptr;
    }

    bool HasContact(int id) const {
        for (const Node<Group>* g = groups_.First(); g; g = g->next)
            for (const Node<Contact>* c = g->data.contacts.First(); c; c = c->next)
                if (c->data.id == id) return true;
        return false;
    }

    bool AddGroup(int id, const std::string& name) {
        if (id <= 0 || name.empty() || FindGroup(id)) return false;
        groups_.Append(Group(id, name));
        return true;
    }

    bool DeleteGroup(int id) {
        return FindGroup(id) && groups_.DeleteCurrent();
    }

    bool AddContact(int groupId, const Contact& contact) {
        Group* g = FindGroup(groupId);
        if (!g || contact.id <= 0 || contact.name.empty() || contact.phone.empty()
            || HasContact(contact.id)) return false;
        g->contacts.Append(contact);
        return true;
    }

    bool UpdateContact(int groupId, const Contact& value) {
        Group* g = FindGroup(groupId);
        if (!g || value.name.empty() || value.phone.empty()
            || !g->contacts.Locate(value.id, true)) return false;
        g->contacts.CurData() = value; // 保留编号，修改其它信息
        return true;
    }

    bool DeleteContact(int groupId, int id) {
        Group* g = FindGroup(groupId);
        return g && g->contacts.Locate(id, true) && g->contacts.DeleteCurrent();
    }

    bool MoveContact(int sourceId, int targetId, int contactId) {
        if (sourceId == targetId) return false;
        Group* source = FindGroup(sourceId);
        Group* target = FindGroup(targetId);
        if (!source || !target || !source->contacts.Locate(contactId, true))
            return false;
        // 先追加成功再删除，避免追加失败时丢失联系人。
        target->contacts.Append(source->contacts.CurData());
        source->contacts.DeleteCurrent();
        return true;
    }

    std::size_t ContactCount() const {
        std::size_t total = 0;
        for (const Node<Group>* g = groups_.First(); g; g = g->next)
            total += g->data.contacts.NumNodes();
        return total;
    }

    std::size_t Search(const std::string& keyword, std::ostream& out) const {
        std::size_t count = 0;
        // 两重遍历：先访问外层分组，再访问这个分组的内层联系人。
        for (const Node<Group>* g = groups_.First(); g; g = g->next) {
            for (const Node<Contact>* p = g->data.contacts.First(); p; p = p->next) {
                const Contact& c = p->data;
                if (std::to_string(c.id) == keyword
                    || c.name.find(keyword) != std::string::npos
                    || c.phone.find(keyword) != std::string::npos
                    || c.email.find(keyword) != std::string::npos) {
                    out << "[" << g->data.name << "] " << c << '\n';
                    ++count;
                }
            }
        }
        return count;
    }

    // 分层保存：G 是分组记录，C 是其下的联系人记录。
    // quoted 允许文本含空格、双引号和反斜杠。
    bool Save(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out) return false;
        out << "GROUPED_ADDRESS_BOOK_V1\n" << groups_.NumNodes() << '\n';
        for (const Node<Group>* g = groups_.First(); g; g = g->next) {
            out << "G " << g->data.id << ' ' << std::quoted(g->data.name)
                << ' ' << g->data.contacts.NumNodes() << '\n';
            for (const Node<Contact>* p = g->data.contacts.First(); p; p = p->next) {
                const Contact& c = p->data;
                out << "C " << c.id << ' ' << std::quoted(c.name)
                    << ' ' << std::quoted(c.phone) << ' ' << std::quoted(c.email) << '\n';
            }
        }
        out.close();
        return static_cast<bool>(out);
    }

    // 在临时通讯录中校验，全部成功后才替换；失败不会清空现有数据。
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
        if (!in.eof()) return false; // 拒绝尾部多余数据
        groups_.Swap(temp.groups_);
        return true;
    }
};

#endif
