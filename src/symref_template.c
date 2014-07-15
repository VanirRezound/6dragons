/** INSERT headers */

SPELL_FUN              *spell_function( char *name )
{
    /** INSERT spell_function name **/
    if ( !str_cmp( name, "reserved" ) )
        return NULL;
    return spell_notfound;
}

DO_FUN                 *skill_function args( ( char *name ) )
{
    /** INSERT skill_function name **/
    return skill_notfound;
}

const char             *spell_name( SPELL_FUN *spell )
{
    /** INSERT spell_name spell **/
    return "reserved";
}

const char             *skill_name( DO_FUN *skill )
{
    static char             buf[64];

    if ( skill == NULL )
        return "reserved";

    /** INSERT skill_name skill **/

    snprintf( buf, 64, "(%p)", skill );
    return buf;
}
