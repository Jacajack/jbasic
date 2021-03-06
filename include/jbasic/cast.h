#ifndef JBAS_CAST_H
#define JBAS_CAST_H

#include <jbasic/defs.h>
#include <jbasic/token.h>

jbas_error jbas_symbol_to_resource(jbas_env *env, jbas_token *t);
jbas_error jbas_to_value(jbas_env *env, jbas_token *t);


jbas_number_type jbas_number_type_promotion(jbas_number_type a, jbas_number_type b);
void jbas_number_cast(jbas_number_token *n, jbas_number_type t);
jbas_error jbas_token_to_number(jbas_env *env, jbas_token *t);
jbas_error jbas_token_to_number_type(jbas_env *env, jbas_token *t, jbas_number_type type);
bool jbas_can_cast_to_number(jbas_token *t);

#endif