/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <stdio.h>
#include <prthread.h>
#include "nsExpirationTracker.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsXPCOM.h"
#include "nsILocalFile.h"
#include "prinrval.h"
#include "nsThreadUtils.h"

namespace TestExpirationTracker {

struct Object {
  Object() : mExpired(false) { Touch(); }
  void Touch() { mLastUsed = PR_IntervalNow(); mExpired = false; }

  nsExpirationState mExpiration;
  nsExpirationState* GetExpirationState() { return &mExpiration; }

  PRIntervalTime mLastUsed;
  bool           mExpired;
};

static bool error;
static PRUint32 periodMS = 100;
static PRUint32 ops = 1000;
static PRUint32 iterations = 2;
static bool logging = 0;
static PRUint32 sleepPeriodMS = 50;
static PRUint32 slackMS = 20; // allow this much error

static void SignalError() {
  printf("ERROR!\n");
  error = true;
}

template <PRUint32 K> class Tracker : public nsExpirationTracker<Object,K> {
public:
  Tracker() : nsExpirationTracker<Object,K>(periodMS) {
    Object* obj = new Object();
    mUniverse.AppendElement(obj);
    LogAction(obj, "Created");
  }

  nsTArray<nsAutoArrayPtr<Object> > mUniverse;

  void LogAction(Object* aObj, const char* aAction) {
    if (logging) {
      printf("%d %p(%d): %s\n", PR_IntervalNow(),
             static_cast<void*>(aObj), aObj->mLastUsed, aAction);
    }
  }

  void DoRandomOperation() {
    Object* obj;
    switch (rand() & 0x7) {
    case 0: {
      if (mUniverse.Length() < 50) {
        obj = new Object();
        mUniverse.AppendElement(obj);
        nsExpirationTracker<Object,K>::AddObject(obj);
        LogAction(obj, "Created and added");
      }
      break;
    }
    case 4: {
      if (mUniverse.Length() < 50) {
        obj = new Object();
        mUniverse.AppendElement(obj);
        LogAction(obj, "Created");
      }
      break;
    }
    case 1: {
      obj = mUniverse[PRUint32(rand())%mUniverse.Length()];
      if (obj->mExpiration.IsTracked()) {
        nsExpirationTracker<Object,K>::RemoveObject(obj);
        LogAction(obj, "Removed");
      }
      break;
    }
    case 2: {
      obj = mUniverse[PRUint32(rand())%mUniverse.Length()];
      if (!obj->mExpiration.IsTracked()) {
        obj->Touch();
        nsExpirationTracker<Object,K>::AddObject(obj);
        LogAction(obj, "Added");
      }
      break;
    }
    case 3: {
      obj = mUniverse[PRUint32(rand())%mUniverse.Length()];
      if (obj->mExpiration.IsTracked()) {
        obj->Touch();
        nsExpirationTracker<Object,K>::MarkUsed(obj);
        LogAction(obj, "Marked used");
      }
      break;
    }
    }
  }
  
protected:
  void NotifyExpired(Object* aObj) {
    LogAction(aObj, "Expired");
    PRIntervalTime now = PR_IntervalNow();
    PRUint32 timeDiffMS = (now - aObj->mLastUsed)*1000/PR_TicksPerSecond();
    // See the comment for NotifyExpired in nsExpirationTracker.h for these
    // bounds
    PRUint32 lowerBoundMS = (K-1)*periodMS - slackMS;
    PRUint32 upperBoundMS = K*(periodMS + sleepPeriodMS) + slackMS;
    if (logging) {
      printf("Checking: %d-%d = %d [%d,%d]\n",
             now, aObj->mLastUsed, timeDiffMS, lowerBoundMS, upperBoundMS);
    }
    if (timeDiffMS < lowerBoundMS || timeDiffMS > upperBoundMS) {
      if (timeDiffMS < periodMS && aObj->mExpired) {
        // This is probably OK, it probably just expired twice
      } else {
        SignalError();
      }
    }
    aObj->Touch();
    aObj->mExpired = true;
    DoRandomOperation();
    DoRandomOperation();
    DoRandomOperation();
  }
};

template <PRUint32 K> static bool test_random() {
  srand(K);
  error = false;
 
  for (PRUint32 j = 0; j < iterations; ++j) {
    Tracker<K> tracker;

    PRUint32 i = 0;
    for (i = 0; i < ops; ++i) {
      if ((rand() & 0xF) == 0) {
        // Simulate work that takes time
        if (logging) {
          printf("SLEEPING for %dms (%d)\n", sleepPeriodMS, PR_IntervalNow());
        }
        PR_Sleep(PR_MillisecondsToInterval(sleepPeriodMS));
        // Process pending timer events
        NS_ProcessPendingEvents(nsnull);
      }
      tracker.DoRandomOperation();
    }
  }
  
  return !error;
}

static bool test_random3() { return test_random<3>(); }
static bool test_random4() { return test_random<4>(); }
static bool test_random8() { return test_random<8>(); }

typedef bool (*TestFunc)();
#define DECL_TEST(name) { #name, name }

static const struct Test {
  const char* name;
  TestFunc    func;
} tests[] = {
  DECL_TEST(test_random3),
  DECL_TEST(test_random4),
  DECL_TEST(test_random8),
  { nsnull, nsnull }
};

}

using namespace TestExpirationTracker;

int main(int argc, char **argv) {
  int count = 1;
  if (argc > 1)
    count = atoi(argv[1]);

  if (NS_FAILED(NS_InitXPCOM2(nsnull, nsnull, nsnull)))
    return -1;

  while (count--) {
    for (const Test* t = tests; t->name != nsnull; ++t) {
      printf("%25s : %s\n", t->name, t->func() ? "SUCCESS" : "FAILURE");
    }
  }
  
  NS_ShutdownXPCOM(nsnull);
  return 0;
}
