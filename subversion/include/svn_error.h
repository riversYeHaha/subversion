/* svn_error.h:  common exception handling for Subversion
 *
 * ================================================================
 * Copyright (c) 2000 CollabNet.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * 3. The end-user documentation included with the redistribution, if
 * any, must include the following acknowlegement: "This product includes
 * software developed by CollabNet (http://www.Collab.Net)."
 * Alternately, this acknowlegement may appear in the software itself, if
 * and wherever such third-party acknowlegements normally appear.
 * 
 * 4. The hosted project names must not be used to endorse or promote
 * products derived from this software without prior written
 * permission. For written permission, please contact info@collab.net.
 * 
 * 5. Products derived from this software may not use the "Tigris" name
 * nor may "Tigris" appear in their names without prior written
 * permission of CollabNet.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL COLLABNET OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 * 
 * This software may consist of voluntary contributions made by many
 * individuals on behalf of CollabNet.
 */




#ifndef SVN_ERROR_H
#define SVN_ERROR_H


#include <svn_types.h>
#include <svn_string.h>
#include <apr_errno.h>     /* APR's error system */
#include <stdio.h>


#define SVN_NO_ERROR   0   /* the best kind of (svn_error_t *) ! */

/* 
   Define custom Subversion error numbers, in the range reserved for
   that in APR: from APR_OS_START_USEERR to APR_OS_START_SYSERR (see
   apr_errno.h).
*/
typedef enum svn_errno_t {
  SVN_WARNING = (APR_OS_START_USEERR + 1),
  SVN_ERR_NOT_AUTHORIZED,
  SVN_ERR_PLUGIN_LOAD_FAILURE,
  SVN_ERR_UNKNOWN_FS_ACTION,
  SVN_ERR_UNEXPECTED_EOF,
  SVN_ERR_MALFORMED_FILE,
  SVN_ERR_INCOMPLETE_DATA,

  /* The xml delta we got was not valid. */
  SVN_ERR_MALFORMED_XML,

  /* A working copy "descent" crawl came up empty */
  SVN_ERR_UNFRUITFUL_DESCENT,

  /* A bogus filename was passed to a routine */
  SVN_ERR_BAD_FILENAME,

  /* There's no such xml tag attribute */
  SVN_ERR_XML_ATTRIB_NOT_FOUND,

  /* Can't do this update or checkout, because something was in the way. */
  SVN_ERR_OBSTRUCTED_UPDATE,

  /* A mismatch popping the wc unwind stack. */
  SVN_ERR_WC_UNWIND_MISMATCH,

  /* Trying to pop an empty unwind stack. */
  SVN_ERR_WC_UNWIND_EMPTY,

  /* Trying to unlock when there's non-empty unwind stack. */
  SVN_ERR_WC_UNWIND_NOT_EMPTY,

  /* A general filesystem error.  */
  SVN_ERR_FS_GENERAL,

  /* An error occurred while trying to close a Subversion filesystem.
     This status code is meant to be returned from APR pool cleanup
     functions; since that interface doesn't allow us to provide more
     detailed information, this is all you'll get.  */
  SVN_ERR_FS_CLEANUP,

  /* You called svn_fs_newfs or svn_fs_open, but the filesystem object
     you provided already refers to some filesystem.  You should allocate
     a fresh filesystem object with svn_fs_new, and use that instead.  */
  SVN_ERR_FS_ALREADY_OPEN,

  /* The error is a Berkeley DB error.  `src_err' is the Berkeley DB
     error code, and `message' is an error message.  */
  SVN_ERR_BERKELEY_DB,

  /* What happens if a non-blocking call to svn_wc__lock() encounters
     another lock. */
  SVN_ERR_WC_LOCKED,

  /* a bad URL was passed to the repository access layer */
  SVN_ERR_ILLEGAL_URL,

  /* the repository access layer could not initialize the socket layer */
  SVN_ERR_SOCK_INIT,

  /* an unsuitable container-pool was passed to svn_make_pool() */
  SVN_ERR_BAD_CONTAINING_POOL

} svn_errno_t;



/*** Wrappers around APR pools, so we get error pools. ***/

/* You may be wondering why is this is in svn_error, instead of
   svn_pool or whatever.  The reason is the needs of the SVN error
   system are our only justification for wrapping APR's pool creation
   funcs -- because errors have to live as long as the top-most pool
   in a test program or a `request'.  If you're not using SVN errors,
   there's no reason not to use APR's native pool interface.  But you
   are using SVN errors, aren't you? */

/* Return a new pool.  If CONTAINING_POOL is non-null, then the new
 * pool will be a subpool of it, and will inherit the containing
 * pool's dedicated error subpool.
 *
 * If anything goes wrong, *ABORT_FUNC will be invoked with the
 * appropriate APR error code, or else a default abort function which
 * exits the program will be run.
 */
apr_pool_t *svn_pool_create (apr_pool_t *containing_pool,
                             int (*abort_func) (int retcode));




/*** SVN error creation and destruction. ***/

typedef struct svn_error
{
  apr_status_t apr_err;      /* APR error value, possibly SVN_ custom err */
  int src_err;               /* native error code (e.g. errno, h_errno...) */
  const char *message;       /* details from producer of error */
  struct svn_error *child;   /* ptr to the error we "wrap" */
  apr_pool_t *pool;          /* The pool holding this error and any
                                child errors it wraps */
} svn_error_t;




/*
  svn_create_error() : for creating nested exception structures.

  Input:  an APR or SVN custom error code,
          the original errno,
          a "child" error to wrap,
          a pool
          a descriptive message,

  Returns:  a new error structure (containing the old one).

  Notes: If POOL is present, the error will always be allocated in a
         new subpool of POOL.  If POOL is null but CHILD is present,
         then it will be allocated in a CHILD's pool (thus
         guaranteeing that the new error has the same lifetime as the
         error it wraps).  At least one of POOL or CHILD must be
         present.

         If creating the "bottommost" error in a chain, pass NULL for
         the child argument.
 */
svn_error_t *svn_create_error (apr_status_t apr_err,
                               int src_err,
                               svn_error_t *child,
                               apr_pool_t *pool,
                               const char *message);

/* Create an error structure with the given APR_ERR, SRC_ERR, CHILD,
   and POOL, with a printf-style error message produced by passing
   FMT, ... through apr_psprintf.  */
extern svn_error_t *svn_create_errorf (apr_status_t apr_err,
                                       int src_err,
                                       svn_error_t *child,
                                       apr_pool_t *pool,
                                       const char *fmt, 
                                       ...);


/* A quick n' easy way to create a wrappered exception with your own
   message, before throwing it up the stack.  (It uses all of the
   child's fields.)  */

svn_error_t *svn_quick_wrap_error (svn_error_t *child, const char *new_msg);


/* Free ERROR by destroying its pool; note that the pool may be shared
   with wrapped child errors inside this error. */
void svn_free_error (svn_error_t *error);


/* Very dumb "default" error handler that anyone can use if they wish. */

void svn_handle_error (svn_error_t *error, FILE *stream);

/* Very dumb "default" warning handler -- used by all policies, unless
   svn_svr_warning_callback() is used to set the warning handler
   differently.  */

void svn_handle_warning (void *data, char *fmt, ...);


#endif   /* SVN_ERROR_H */


/* 
 * local variables:
 * eval: (load-file "../svn-dev.el")
 * end:
 */
