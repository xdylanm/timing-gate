#!/usr/bin/python3


if __name__ == "__main__":
    web_path = "web/"
    html_file = "index.html"
    js_file = "sw.js"
    css_file = "style.css"

    template_file = "htmltemplate.cpp"

    with open(web_path+html_file,"r") as fhtml:
        html = fhtml.readlines()

    with open("sketches/timinggatecontroller/{}".format(template_file),"w") as hf:
        hf.write('#include "htmltemplate.h"\n\n')
        hf.write('namespace HTML\n{\n\n')
        hf.write('const char PROGMEM index[] = R"rawliteral(\n')
        for ln in html:
            if ln.count(css_file) > 0:
                hf.write('    <style>\n')
                with open(web_path+css_file, "r") as css:
                    for ln_css in css.readlines():
                        hf.write('      {}'.format(ln_css))
                hf.write('    </style>\n')
            elif ln.count(js_file) > 0:
                hf.write('    <script>\n')
                with open(web_path+js_file, "r") as js:
                    for ln_js in js.readlines():
                        hf.write('      {}'.format(ln_js))
                hf.write('    </script>\n')
            else:
                hf.write(ln)
        hf.write('\n)rawliteral";\n\n} // namespace HTML\n')





