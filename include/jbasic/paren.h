#ifndef JBAS_PAREN_H
#define JBAS_PAREN_H

#include <jbasic/defs.h>
#include <jbasic/token.h>

bool jbas_is_paren(const jbas_token *t);
jbas_error jbas_remove_entire_paren(jbas_env *env, jbas_token *t);
jbas_error jbas_remove_paren(jbas_env *env, jbas_token *t);
jbas_error jbas_eval_paren(jbas_env *env, jbas_token *t);

#endif