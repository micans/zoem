# zoem
A macro/programming language with stacks

It looks like this:
```
\def{fib#1}{
   \push{fibonacci}
   \set{a}{1}
   \set{b}{1}
   \set{c}{0}
   \while{\let{\a <= \1}}{
      \setx{c}{\a}
      \setx{a}{\let{\a + \b}}
      \write{-}{txt}{\c\|}
      \setx{b}{\c}
   }
   \pop{fibonacci}
}

\: need to escape newlines below, lest they ruin the prompt

\write{-}{device}{Enter a number please, then press , \@{\N>\s}}\
\setx{num}{\zinsert{-}}\
\fib{\num}
```

Zoem has stacks, arithmetic environment, delayed evaluation, inside-out evaluation,
and a lot more features to allow abstraction, encapsulation and (lexical) scoping, as well
as syntactic sugar to facilitate HTML production.

Built deeply into the zoem language is that it allows distinction between two
types of scope; plain scope where text/output is assumed to carry meaning, and
device scope, where text/output is assumed to be mark-up. Different filtering
mechanisms exist for the two scopes. This facilitates the creation of compact
DSL languages that support completely different output formats (e.g. HTML and
nroff), such as [Portable Unix Documentation](https://micans.org/pud/).

For more information visit [Zoem's homepage](https://micans.org/zoem).

