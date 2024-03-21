#pragma once
#include <list>
#include <QMutex>

template<typename T>
class TMessageQue
{
public:

    void push_back(T const& msg)
    {
        m_mutex.lock();
        m_list.push_back(msg);
        m_mutex.unlock();
    }

    bool get(T& msg)
    {
        bool nRet = false;
        m_mutex.lock();
        if (!m_list.empty())
        {
            msg = m_list.front();
            m_list.pop_front();
            nRet = true;
        }
        m_mutex.unlock();

        return nRet;
    }

    int size()
    {
        m_mutex.lock();
        int size = (int)m_list.size();
        m_mutex.unlock();

        return size;
    }

    void clear()
    {
        m_mutex.lock();
        m_list.clear();
        m_mutex.unlock();
    }
private:

    QMutex					m_mutex;
    std::list<T>			m_list;
};
