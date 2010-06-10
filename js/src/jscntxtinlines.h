/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is SpiderMonkey code.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jeff Walden <jwalden+code@mit.edu> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jscntxtinlines_h___
#define jscntxtinlines_h___

#include "jscntxt.h"
#include "jsparse.h"
#include "jsxml.h"

#include "jsobjinlines.h"

inline bool
JSContext::ensureGeneratorStackSpace()
{
    bool ok = genStack.reserve(genStack.length() + 1);
    if (!ok)
        js_ReportOutOfMemory(this);
    return ok;
}

namespace js {

JS_REQUIRES_STACK JS_ALWAYS_INLINE JSStackFrame *
CallStack::getCurrentFrame() const
{
    JS_ASSERT(inContext());
    return isSuspended() ? getSuspendedFrame() : cx->fp;
}

JS_REQUIRES_STACK inline jsval *
StackSpace::firstUnused() const
{
    CallStack *ccs = currentCallStack;
    if (!ccs)
        return base;
    if (JSContext *cx = ccs->maybeContext()) {
        if (!ccs->isSuspended())
            return cx->regs->sp;
        return ccs->getSuspendedRegs()->sp;
    }
    return ccs->getInitialArgEnd();
}

/* Inline so we don't need the friend API. */
JS_ALWAYS_INLINE void
StackSpace::assertIsCurrent(JSContext *cx) const
{
#ifdef DEBUG
    JS_ASSERT(cx == currentCallStack->maybeContext());
    JS_ASSERT(cx->getCurrentCallStack() == currentCallStack);
    cx->assertCallStacksInSync();
#endif
}

JS_ALWAYS_INLINE bool
StackSpace::ensureSpace(JSContext *maybecx, jsval *from, ptrdiff_t nvals) const
{
    JS_ASSERT(from == firstUnused());
#ifdef XP_WIN
    JS_ASSERT(from <= commitEnd);
    if (commitEnd - from >= nvals)
        return true;
    if (end - from < nvals) {
        if (maybecx)
            js_ReportOutOfScriptQuota(maybecx);
        return false;
    }
    if (!bumpCommit(from, nvals)) {
        if (maybecx)
            js_ReportOutOfScriptQuota(maybecx);
        return false;
    }
    return true;
#else
    if (end - from < nvals) {
        if (maybecx)
            js_ReportOutOfScriptQuota(maybecx);
        return false;
    }
    return true;
#endif
}

JS_ALWAYS_INLINE bool
StackSpace::ensureEnoughSpaceToEnterTrace()
{
#ifdef XP_WIN
    return ensureSpace(NULL, firstUnused(), MAX_TRACE_SPACE_VALS);
#endif
    return end - firstUnused() > MAX_TRACE_SPACE_VALS;
}

JS_REQUIRES_STACK JS_ALWAYS_INLINE JSStackFrame *
StackSpace::getInlineFrame(JSContext *cx, jsval *sp,
                           uintN nmissing, uintN nfixed) const
{
    assertIsCurrent(cx);
    JS_ASSERT(cx->hasActiveCallStack());
    JS_ASSERT(cx->regs->sp == sp);

    ptrdiff_t nvals = nmissing + VALUES_PER_STACK_FRAME + nfixed;
    if (!ensureSpace(cx, sp, nvals))
        return NULL;

    JSStackFrame *fp = reinterpret_cast<JSStackFrame *>(sp + nmissing);
    return fp;
}

JS_REQUIRES_STACK JS_ALWAYS_INLINE void
StackSpace::pushInlineFrame(JSContext *cx, JSStackFrame *fp, jsbytecode *pc,
                            JSStackFrame *newfp)
{
    assertIsCurrent(cx);
    JS_ASSERT(cx->hasActiveCallStack());
    JS_ASSERT(cx->fp == fp && cx->regs->pc == pc);

    fp->savedPC = pc;
    newfp->down = fp;
#ifdef DEBUG
    newfp->savedPC = JSStackFrame::sInvalidPC;
#endif
    cx->setCurrentFrame(newfp);
}

JS_REQUIRES_STACK JS_ALWAYS_INLINE void
StackSpace::popInlineFrame(JSContext *cx, JSStackFrame *up, JSStackFrame *down)
{
    assertIsCurrent(cx);
    JS_ASSERT(cx->hasActiveCallStack());
    JS_ASSERT(cx->fp == up && up->down == down);
    JS_ASSERT(up->savedPC == JSStackFrame::sInvalidPC);

    JSFrameRegs *regs = cx->regs;
    regs->pc = down->savedPC;
    regs->sp = up->argv - 1;
#ifdef DEBUG
    down->savedPC = JSStackFrame::sInvalidPC;
#endif
    cx->setCurrentFrame(down);
}

void
AutoIdArray::trace(JSTracer *trc) {
    JS_ASSERT(tag == IDARRAY);
    js::TraceValues(trc, idArray->length, idArray->vector, "JSAutoIdArray.idArray");
}

class AutoNamespaces : protected AutoGCRooter {
  protected:
    AutoNamespaces(JSContext *cx) : AutoGCRooter(cx, NAMESPACES) {
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  public:
    JSXMLArray array;
};

inline void
AutoGCRooter::trace(JSTracer *trc)
{
    switch (tag) {
      case JSVAL:
        JS_SET_TRACING_NAME(trc, "js::AutoValueRooter.val");
        js_CallValueTracerIfGCThing(trc, static_cast<AutoValueRooter *>(this)->val);
        return;

      case SPROP:
        static_cast<AutoScopePropertyRooter *>(this)->sprop->trace(trc);
        return;

      case WEAKROOTS:
        static_cast<AutoPreserveWeakRoots *>(this)->savedRoots.mark(trc);
        return;

      case PARSER:
        static_cast<Parser *>(this)->trace(trc);
        return;

      case SCRIPT:
        if (JSScript *script = static_cast<AutoScriptRooter *>(this)->script)
            js_TraceScript(trc, script);
        return;

      case ENUMERATOR:
        static_cast<AutoEnumStateRooter *>(this)->trace(trc);
        return;

      case IDARRAY: {
        JSIdArray *ida = static_cast<AutoIdArray *>(this)->idArray;
        TraceValues(trc, ida->length, ida->vector, "js::AutoIdArray.idArray");
        return;
      }

      case DESCRIPTORS: {
        PropertyDescriptorArray &descriptors =
            static_cast<AutoDescriptorArray *>(this)->descriptors;
        for (size_t i = 0, len = descriptors.length(); i < len; i++) {
            PropertyDescriptor &desc = descriptors[i];

            JS_CALL_VALUE_TRACER(trc, desc.pd, "PropertyDescriptor::pd");
            JS_CALL_VALUE_TRACER(trc, desc.value, "PropertyDescriptor::value");
            JS_CALL_VALUE_TRACER(trc, desc.get, "PropertyDescriptor::get");
            JS_CALL_VALUE_TRACER(trc, desc.set, "PropertyDescriptor::set");
            js_TraceId(trc, desc.id);
        }
        return;
      }

      case DESCRIPTOR : {
        AutoDescriptor &desc = *static_cast<AutoDescriptor *>(this);
        if (desc.obj)
            JS_CALL_OBJECT_TRACER(trc, desc.obj, "Descriptor::obj");
        JS_CALL_VALUE_TRACER(trc, desc.value, "Descriptor::value");
        if (desc.attrs & JSPROP_GETTER)
            JS_CALL_VALUE_TRACER(trc, jsval(desc.getter), "Descriptor::get");
        if (desc.attrs & JSPROP_SETTER)
            JS_CALL_VALUE_TRACER(trc, jsval(desc.setter), "Descriptor::set");
        return;
      }

      case NAMESPACES: {
        JSXMLArray &array = static_cast<AutoNamespaces *>(this)->array;
        TraceObjectVector(trc, reinterpret_cast<JSObject **>(array.vector), array.length);
        array.cursors->trace(trc);
        return;
      }

      case XML:
        js_TraceXML(trc, static_cast<AutoXMLRooter *>(this)->xml);
        return;

      case OBJECT:
        if (JSObject *obj = static_cast<AutoObjectRooter *>(this)->obj) {
            JS_SET_TRACING_NAME(trc, "js::AutoObjectRooter.obj");
            js_CallGCMarker(trc, obj, JSTRACE_OBJECT);
        }
        return;

      case ID:
        JS_SET_TRACING_NAME(trc, "js::AutoIdRooter.val");
        js_CallValueTracerIfGCThing(trc, static_cast<AutoIdRooter *>(this)->idval);
        return;

      case VECTOR: {
        js::Vector<jsval, 8> &vector = static_cast<js::AutoValueVector *>(this)->vector;
        js::TraceValues(trc, vector.length(), vector.begin(), "js::AutoValueVector.vector");
        return;
      }
    }

    JS_ASSERT(tag >= 0);
    TraceValues(trc, tag, static_cast<AutoArrayRooter *>(this)->array, "js::AutoArrayRooter.array");
}

}

#endif /* jscntxtinlines_h___ */