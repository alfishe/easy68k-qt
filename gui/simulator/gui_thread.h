// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Running code on the GUI thread from the simulator thread.

#ifndef EASY68K_GUI_SIMULATOR_GUI_THREAD_H_
#define EASY68K_GUI_SIMULATOR_GUI_THREAD_H_

#include <QMetaObject>
#include <QObject>
#include <QThread>
#include <utility>

namespace easym68k::gui {

// Runs `fn` on `context`'s thread without waiting (directly when called on
// that thread). Calls are executed in order.
template <typename Fn>
void PostToThread(QObject* context, Fn fn) {
  if (QThread::currentThread() == context->thread())
    fn();
  else
    QMetaObject::invokeMethod(context, std::move(fn), Qt::QueuedConnection);
}

// Runs `fn` on `context`'s thread and returns its result, blocking the
// caller (directly when called on that thread).
template <typename R, typename Fn>
R CallInThread(QObject* context, Fn fn) {
  if (QThread::currentThread() == context->thread())
    return fn();
  R result{};
  QMetaObject::invokeMethod(
      context, [&result, &fn] { result = fn(); }, Qt::BlockingQueuedConnection);
  return result;
}

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_GUI_THREAD_H_
