/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.sync.repositories;

import org.mozilla.gecko.sync.SyncException;

public class InvalidSessionTransitionException extends SyncException {

  private static final long serialVersionUID = 4157729859314427281L;

  public InvalidSessionTransitionException(Exception ex) {
    super(ex);
  }

}
