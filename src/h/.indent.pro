/*
 * RAM $Id$
 */

--line-length90					/* l    - Overall width of code */
--comment-line-length90				/* lc   - width of comment lines */
--declaration-indentation24			/* di   - variable declarations line up here */
--comment-indentation56				/* c    - line up comments at column N */
--declaration-comment-column56			/* cd   - line up comments after declarations in column N */

--blank-lines-after-declarations		/* bad  - space out declarations */
--blank-lines-after-procedures			/* bap  - space out bodies */
--break-before-boolean-operator			/* bbo  - break before boolean operators if possible */
--blank-lines-after-commas			/* bc   - seperate lines for comma seperated declarations */
--braces-after-if-line				/* bl   - place opening brace AFTER the if line */
--brace-indent0					/* bli0 - leave braces flush with statements they belong to */
--braces-after-struct-decl-line			/* bls  - put opening brace AFTER struct declaration line */
--case-brace-indentation4			/* cbi4 - indent case braces 4 spaces */
--case-indentation4				/* cli4 - intent case statements 4 spaces */
--honour-newlines				/* hnl  - break lines where it was already broken */
--indent-level4					/* i4   - indent level set to 4 spaces */
--parameter-indentation4			/* ip4  - indent old-style parameter declarations 4 spaces */
--no-tabs					/* nut  - replace tabs by spaces in indentaton */
--no-space-after-function-call-names		/* npcs - function() not function () */
--dont-break-procedure-type			/* npsl - keep function type on the same line as the name */
--space-after-parentheses			/* prs  - ( expression ) not (expression) */
--space-after-for				/* saf  - for ( expression ) not for( expression ) */
--space-after-if				/* sai  - if ( expression ) not if( expression ) */
--space-after-while				/* saw  - while ( expression ) not while( expression ) */
--struct-brace-indentation0			/* sbi0 - leave braces flush with structure definition */

--space-after-casts				/* cs   - ( type ) cast not ( type )cast */
--swallow-optional-blank-lines			/* sob  - replace multiple blank lines with one */
--dont-space-special-semicolon			/* nss  - leave stand-alone semi-colons alone */
--continue-at-parentheses			/* cl   - line up continuations at parens */
--preserve-mtime				/* pmt  - preserve modification time of original files */

--start-left-side-of-comments			/* sc   - add stars in block comments */
--comment-delimiters-on-blank-lines		/* cdb  - put the comment symbols on their own lines */
--format-all-comments				/* fca  - try to reformat comments */
--dont-format-first-column-comments		/* nfc1 - unless they are on the left! */

/* Typedefs should be listed here */
/* sha256.h */

-T SHA256_CTX

/* merc.h */

-T sh_int
-T bool

-T AFFECT_DATA
-T AREA_DATA
-T BAN_DATA
-T BUFFER
-T CHAR_DATA
-T DESCRIPTOR_DATA
-T EXIT_DATA
-T EXTRA_DESCR_DATA
-T HELP_DATA
-T KILL_DATA
-T MEM_DATA
-T MOB_INDEX_DATA
-T NOTE_DATA
-T OBJ_DATA
-T OBJ_INDEX_DATA
-T PC_DATA
-T GEN_DATA
-T RESET_DATA
-T ROOM_INDEX_DATA
-T SHOP_DATA
-T TIME_INFO_DATA
-T WEATHER_DATA

-T DO_FUN
-T SPEC_FUN
-T SPELL_FUN



