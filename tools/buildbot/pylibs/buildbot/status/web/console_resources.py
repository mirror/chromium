from string import Template

top_header = Template('''
<center>
  <table width=95% class="Grid" border="0" cellspacing="0">
''')

top_info_name = Template('''
    <tr>
      <td width=33% align=left class=left_align><a href="$projectUrl">$projectName</a>
''')

top_info_categories = Template('''
        <br><b>Categories:</b> $categories
''')

top_info_branch = Template('''
        <br /><b>Branch:</b> $branch
''')

top_info_name_end = Template('''
      </td>
''')

top_legend = Template('''
<td width=33% align=center class=center_align>
  <center>
    <table>
      <tr>
        <td>Legend:&nbsp;&nbsp;</td>
        <td><div class='legend success' title='All tests passed'>Passed</div></td>
        <td><div class='legend failure' title='There is a new failure. Take a look!'>Failed</div></td>
        <td><div class='legend warnings' title='It was failing before, and it is still failing. Make sure you did not introduce new regressions'>Failed Again</div></td>
        <td><div class='legend running' title='The tests are still running'>Running</div></td>
        <td><div class='legend exception' title='Something went wrong with the test, there is no result'>Exception</div></td>
        <td><div class='legend notstarted' title='No result yet.'>No data</div></td>
      </tr>
    </table>
  </center>
</td>
''')

top_personalize = Template('''
<td width=33% align=right class=right_align>
  <script>
    function reload_page() {
      if (document.location.href.lastIndexOf('?') == -1)
        document.location.href = document.location.href+ '?name=' + chosename.value;
      else
        document.location.href = document.location.href+ '&name=' + chosename.value;
    }
  </script>
  <input id='chosename' name='name' type='text' style='color:#999;'
      onblur='this.value = this.value || this.defaultValue; this.style.color = "#999";'
      onfocus='this.value=""; this.style.color = "#000";'
      value='Personalized for...'>
      
  <input type=submit value='Go' onclick='reload_page()'>
</td>
''')

top_footer = Template('''
    </tr>
  </table>
</center>
''')

main_header = Template('''
<br>
<center>
  <table width=96%>
''')

main_line_info = Template('''
    <tr>
      <td class='DevRev $alt' width=1%>
        <a href='http://src.chromium.org/viewvc/chrome?view=rev&$revision' title='$date'>$revision</a>
      </td>
      <td class='DevName $alt' width=1%>
        $who
      </td>
''')

main_line_status_header = Template('''
      <td class='DevStatus $alt'>
        <table width=100%>
          <tr>
''')

main_line_status_box = Template('''
            <td class='DevStatusBox'>
              <a href='$url' title='$title' class='DevStatusBox $color $tag' target=_blank></a>
            </td>
''')

main_line_status_footer = Template('''
           </tr>
        </table>
      </td>
    </tr>
''')

main_line_details = Template('''
    <tr>
      <td colspan=3 class='DevDetails $alt'> $details </td>
    </tr>
''')

main_line_comments = Template('''
    <tr>
      <td colspan=3 class='DevComment $alt'> $comments </td>
    </tr>
    <tr class='DevStatusSpacing'>
      <td>
      </td>
    </tr>
''')

main_footer = Template('''
  </table>
</center>
''')


bottom = Template('''
<hr />

<div class="footer">
  [ <a href="$welcomeUrl">welcome</a> ] - 
  [ <a class='collapse' href="#" OnClick="collapse();return false;">collapse</a>
   <a class='expand' href="#" style='display: none' OnClick="expand();return false;">expand</a> ] -
  [ <a class='merge' href="#" OnClick="merge();return false;">merge</a>
   <a class='unmerge' style='display: none' href="#" OnClick="unmerge();return false;">un-merge</a> ]
  <br />
  <a href="http://buildbot.sourceforge.net/">Buildbot - $version</a> working for the <a href="$projectUrl"> $projectName </a> project.
  <br />
  Page built: $time
  <br />
  Debug Info: $debugInfo
</div>

<script>
  function createCookie(name, value, day) {
    var date = new Date();
    date.setTime(date.getTime()+(day*24*60*60*1000));
    var expires = "; expires="+date.toGMTString();
    document.cookie = name+"="+value+expires+"; path=/";
  }

  function readCookie(name) {
    var nameEQ = name + "=";
    var ca = document.cookie.split(';');
    for(var i=0; i < ca.length; i++) {
      var c = ca[i];
      while (c.charAt(0) == ' ')
        c = c.substring(1, c.length);
        if (c.indexOf(nameEQ) == 0)
          return c.substring(nameEQ.length, c.length);
	}

    return null;
  }

  function eraseCookie(name) {
    createCookie(name, "", -1);
  }

  function collapse() {
    var comments = document.querySelectorAll('.DevComment');
    for(var i = 0; i < comments.length; i++) {
      comments[i].style.display = "none";
    }

    var details = document.querySelectorAll('.DevDetails');
    for(var i = 0; i < details.length; i++) {
      details[i].style.display = "none";
    }
    
    var revisions = document.querySelectorAll('.DevRev');
    for(var i = 0; i < revisions.length; i++) {
      revisions[i].className = revisions[i].className + ' DevRevCollapse';
    }
    
    var status = document.querySelectorAll('.DevStatus');
    for(var i = 0; i < status.length; i++) {
      status[i].className = status[i].className + ' DevStatusCollapse';
    }
    
    createCookie('collapsed', 'true', 30)
    
    document.querySelectorAll('.collapse')[0].style.display = 'none'
    document.querySelectorAll('.expand')[0].style.display = 'inline'
    document.querySelectorAll('.merge')[0].style.display = 'inline'
    document.querySelectorAll('.unmerge')[0].style.display = 'none'
  }

  function expand() {
    var comments = document.querySelectorAll('.DevComment');
    for(var i = 0; i < comments.length; i++) {
      comments[i].style.display = "";
    }
    
    var details = document.querySelectorAll('.DevDetails');
    for(var i = 0; i < details.length; i++) {
      details[i].style.display = "";
    }

    var revisions = document.querySelectorAll('.DevRev');
    for(var i = 0; i < revisions.length; i++) {
      revisions[i].className = revisions[i].className.replace('DevRevCollapse', '');
    }
    
    var status = document.querySelectorAll('.DevStatus');
    for(var i = 0; i < status.length; i++) {
      status[i].className = status[i].className.replace('DevStatusCollapse', '');
    }
    
    eraseCookie('collapsed')
    eraseCookie('merged')
    
    document.querySelectorAll('.collapse')[0].style.display = 'inline'
    document.querySelectorAll('.expand')[0].style.display = 'none'
    document.querySelectorAll('.merge')[0].style.display = 'inline'
    document.querySelectorAll('.unmerge')[0].style.display = 'none'
  }

  function merge() {
    collapse();

    var spacing = document.querySelectorAll('.DevStatusSpacing');
    for(var i = 0; i < spacing.length; i++) {
      spacing[i].style.display = "none";
    }
    
    var all_tags = [];
    var element;
    for(var i = 0; i < document.all.length; i++) {
      element = document.all[i];
      start = element.className.indexOf('Tag')
      if (start != -1) {
        var class_name = ""
        end = element.className.indexOf(' ', start)
        if (end != -1) {
          class_name = element.className.substring(start, end);
        } else {
          class_name = element.className.substring(start);
        }
        all_tags[class_name] = 1;
      }
    }
    
    for (i in all_tags) {
      var current = document.querySelectorAll('.' + i);
      
      if (current.length > 1) {
        for(var i = 0; i < current.length; i++) {
          current[i].className = current[i].className + ' noround';
        }
      
        current[0].className = current[0].className + ' begin';
        last = current.length - 1;
        current[last].className = current[last].className + ' end';
      }
    }

    document.querySelectorAll('.collapse')[0].style.display = 'none'
    document.querySelectorAll('.expand')[0].style.display = 'inline'
    document.querySelectorAll('.merge')[0].style.display = 'none'
    document.querySelectorAll('.unmerge')[0].style.display = 'inline'

    createCookie('merged', 'true', 30)
  }

  function unmerge() {
    expand();

    var spacing = document.querySelectorAll('.DevStatusSpacing');
    for(var i = 0; i < spacing.length; i++) {
      spacing[i].style.display = "";
    }

    var noround = document.querySelectorAll('.noround');
    for(var i = 0; i < noround.length; i++) {
      noround[i].className = noround[i].className.replace("begin", '');
      noround[i].className = noround[i].className.replace("end", '');
      noround[i].className = noround[i].className.replace("noround", '');
    }    
    
    eraseCookie('merged')

    document.querySelectorAll('.collapse')[0].style.display = 'inline'
    document.querySelectorAll('.expand')[0].style.display = 'none'
    document.querySelectorAll('.merge')[0].style.display = 'inline'
    document.querySelectorAll('.unmerge')[0].style.display = 'none'
  }

  if (readCookie('merged')) {
    merge();
  } else if (readCookie('collapsed')) {
    collapse();
  } else  {
    unmerge();
    expand();
  }

  
</script>

''')
  