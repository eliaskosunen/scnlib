'use strict';

$(document).ready(function() {
    // Find all `<li>` elements inside `#poxy-toc`
    $('#poxy-toc').find('ul').children('li')
        // Additionally, find all headers
        .add('h1, h2, h3, h4, h5')
        // Replace:
        //  - &lt;code&gt;...&lt;/code&gt; -> <code>...</code>
        //  - likewise for <tt> (replace with <code>, too)
        .each(function () {
            $(this).html($(this).html()
                .replace(/&lt;code&gt;(.*?)&lt;\/code&gt;/, '<code>$1</code>')
                .replace(/&lt;tt&gt;(.*?)&lt;\/tt&gt;/, '<code>$1</code>'));
        });
});
