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

For more information visit [Zoem's homepage](https://micans.org/zoem).

