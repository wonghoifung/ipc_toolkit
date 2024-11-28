#include "g_timer.h"

G_Timer::G_Timer():sourceId_(0),user_data_(nullptr) {

}

G_Timer::~G_Timer() {
    stop();
}

static gboolean once_callback(gpointer user_data) {
    G_Timer* gtimer = (G_Timer*)user_data;
    auto cb = gtimer->get_callback();
    cb(gtimer);
    return G_SOURCE_REMOVE;
}

void G_Timer::start_once(int millisec, timeout_callback cb, void* user_data) {
    cb_ = cb; 
    user_data_ = user_data;
    stop();
    sourceId_ = g_timeout_add (millisec, once_callback, this);
}

static gboolean cycle_callback(gpointer user_data) {
    G_Timer* gtimer = (G_Timer*)user_data;
    auto cb = gtimer->get_callback();
    cb(gtimer);
    return G_SOURCE_CONTINUE;
}

void G_Timer::start_cycle(int millisec, timeout_callback cb, void* user_data) {
    cb_ = cb;
    user_data_ = user_data;
    stop();
    sourceId_ = g_timeout_add (millisec, cycle_callback, this);   
}

void G_Timer::stop() {
    if (sourceId_ != 0) {
        g_source_remove(sourceId_);
        sourceId_ = 0;
    }
}
