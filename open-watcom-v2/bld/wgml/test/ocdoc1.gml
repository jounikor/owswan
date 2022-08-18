:GDOC
:BODY
.co on
.ju off
.dc tb >
.co off
     This tests spaces before,    within, and after a line.
.co on
.br;     This tests spaces before,    within, and after a line.

.sk;Parts of this document were copied and modified from the Wiki. The reason
for this was to make it less self-referential. Ironically, we start with some
rather self-referential tests of font switching: the title was in available
font 3; most of the text is in available font 0; here is a phrase in
:HP1.available font 1:eHP1. and here is a phrase in :HP2.available font
2:eHP2., neither including spaces before or behind (the usual situation). The
rarer cases have the highlighted phrase include:HP1. the space before:eHP1.
or :HP2.the space behind :eHP2.(which can affect layout if the space widths
vary by font) and, very rarely, perhaps, to start in the mid:HP3.dle:eHP3. of
a word.

.sk;These are tests of extremely long words. First, a solid multiline word: aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeeeaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee
And now an early multi-part multiline word: aaaaaaaaaa:HP1.aaaaaaaaaa:eHP1.bbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeeeaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee
And now a late multi-part multiline word: aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbccccc:HP1.cccccccccc:eHP1.cccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeeeaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee
Now with early tabbing: aaaaa>aaaaaaaaaa>aaaaabbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeeeaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeee
and: aaaaa>aaaaa:HP1.aaaaa>aaaaa:eHP1.bbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeeeaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeee
Next, with late tabbing: aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee>aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee
and: aaaaaaaaaa:HP1.aaaaaaaaaa:eHP1.bbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee>aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccdddddddddddddddddddddddddddeeeeeeeeeeeeeeee

.sk;Now we get to the test of the stops. Research suggests that there are four:
'.', '!', '?' and '.', at least when the criterion is that wgml 4.0 puts two
spaces after them when they are used with wscript/noscript in effect! This is
surely worth examining, is it not? It is a bit surprising, however, to find
that neither ',' nor ';' is a stop; still, what matters here is what wgml 4.0
does. And putting the stop before a ')' appears to cancel the effect. (Thus,
this will not be followed by two spaces.)
(Nor will this example!)
(And this won't either, will it?)
(This one is, of course, rather unlikely:)
in practice. The same can be said of '"': "Thus,
this will not be followed by two spaces."
"Nor will this example!"
"And this won't either, will it?"
"This one is, of course, rather unlikely:"
in practice.

.sk;Next we test subscripts and superscripts. Unfortunately, wgml 4.0 can only
do this one way: &S'superscript.. All other variants produce output which PS
cannot handle at least some of the time.

.sk;And now we come to wgml tabs. This is a test line with no wgml tabs at all. >This>is>a>test>line>which>is>infested>with>wgml>tabs.
>This>is>a>test>line>which>is>infested>with>wgml>tabs.
And this line shows one>tab, two>>tabs, and three>>>tabs.
Here are tabs>:HP1.before and after:eHP1.>a highlighted phrase. Finally,
here is a set of five     spaces.

