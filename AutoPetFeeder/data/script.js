function onButton() {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "on", true);
  xhttp.send();
}

function HidePassword()
{
  if (document.getElementById("pass").checked == true)
  {
  document.getElementById('Password').type = 'text';
  }else if (document.getElementById("pass").checked == false) {
  document.getElementById('Password').type = 'password';
  }
}

function onlyOne(checkbox_lang) {
  var checkboxes = document.getElementsByName('lang')
  checkboxes.forEach((item) => {
      if (item !== checkbox_lang) item.checked = false
  })
}

function getData()
{
  var Files = ["morning", "evening", "portion", "lang", "ssid", "password"];
  var FilesBrut = new XMLHttpRequest();

  for (var File of Files){
    FilesBrut.open("GET", "/config/"+ File + ".txt", false);
    FilesBrut.onreadystatechange = function ()
    {
      if(FilesBrut.readyState === 4){
        if(FilesBrut.status === 200 || FilesBrut.status === 0){
          var text = FilesBrut.responseText;
          if (File === "lang"){
            $.get("/lang/"+ text + ".json", function(data){
              for (let i=0;; i++){
                var id = data["data"][i]["id"];
                var elem = document.getElementById(id);
                if (elem != null) {
                  elem.textContent = data["data"][i]["text"];
                }
              };
            });
          }else{
            document.getElementById(File).textContent = text;
            document.getElementById(File).value = text;
          }
        }
      }
    }
    FilesBrut.send(null);
  }
}

