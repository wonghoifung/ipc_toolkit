#pragma once

#include "base.h"
#include "noncopyable.h"
#include <glib.h>
#include <functional>

class G_Timer;
typedef std::function<void(G_Timer*)> timeout_callback;

class G_Timer : noncopyable {
public:
    G_Timer();
    ~G_Timer();
    void start_once(int millisec, timeout_callback cb, void* user_data = nullptr);
    void start_cycle(int millisec, timeout_callback cb, void* user_data = nullptr);
    void stop();
    timeout_callback get_callback() {return cb_;}
    void* get_user_data() {return user_data_;}
private:
    guint sourceId_;
    timeout_callback cb_;
    void* user_data_;
};
