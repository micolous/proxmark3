// readline_compat.h -- compatibility methods for readline.
// This is used for filling gaps in libedit's (BSD, OSX) readline
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//
// This may also be used under the same license as libedit (3-clause BSD).

#include <stdlib.h>
#include <strings.h>
#include <readline/readline.h>

#if !defined(RL_READLINE_VERSION) || RL_READLINE_VERSION < 0x0402
// Old versions of GNU Readline.
# error "Please install GNU Readline 4.2a or later, or BSD libedit"

#elif !defined(RL_STATE_READCMD)
// libedit pretends to be readline 4.2a in RL_READLINE_VERSION, but does not
// implement rl_readline_state (which was added in readline 4.2).
//
// libedit doesn't expose enough state out the readline compatibility layer to
// know when readline() is waiting for user input. It doesn't hurt to always
// report that we are handling user input.
#define RL_STATE_READCMD 1
#define rl_readline_state 1

char* _saved_prompt = NULL;
void rl_save_prompt() {
  if (rl_prompt == NULL) {
    return;
  }
  
  if (_saved_prompt != NULL) {
    // Destroy old prompt
    free(_saved_prompt);
  }
  _saved_prompt = strdup(rl_prompt);

  // Destroy the prompt in place
  rl_set_prompt("");
}

void rl_restore_prompt() {
  if (_saved_prompt != NULL) {
    rl_set_prompt(_saved_prompt);
    free(_saved_prompt);
    _saved_prompt = NULL;
  }
}

char *rl_copy_text(start, length) {
  // rl_copy_text is always passed (0, rl_end)
  // But libedit doesn't handle rl_end unless you have a hook that calls
  // _rl_update_pos after each character.
  // And it also doesn't update rl_line_buffer until a command is completed!
  // Return NULL here to signal our fake rl_replace_line to restore the prompt.
  return NULL;
}

void rl_replace_line(const char* text, int clear_undo) {
  if (text == NULL) {
    // Restoring prompt
    // TODO: This restores most of the prompt, but arrow keys are broken for the
    // first keypress after a PrintAndLog, and need to be prefixed with ESC.
    //
    // This might be fixable with newer libedit
    rl_prep_terminal(0);
    rl_forced_update_display();
  } else if (text[0] == '\0') {
    // Clear line
    rl_deprep_terminal();
    printf("\r\x1B[J");    
  }
}
#endif
