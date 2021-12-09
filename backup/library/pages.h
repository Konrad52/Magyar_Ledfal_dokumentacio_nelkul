const char* INDEX_PAGE_1 = R"(
<!DOCTYPE html>
<html lang="hu">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP Settings Page</title>
</head>
<body>
    <table>
)";

const char* INDEX_PAGE_2 = R"=(
    </table>
    <button onclick="location.reload()">Frissítés</button>
    <button onclick="submit()">Beállítás</button>
</body>
<script>
    function submit() {
        var inputs = document.getElementsByTagName('input');
        let data = new FormData();
        Array.from(inputs).forEach(input => {
            if (input.id.startsWith('val')) {
                data.append(input.id, input.value);
            }
        });

        fetch("post", {
            method: 'POST',
            body: data
        });
    }
</script>
<style>
    table, tr, td {
        font-family: Verdana, Geneva, Tahoma, sans-serif;
        font-size: 14px;
        border: 1px solid black;
        background-color: #ddd;
        color: black;
        padding: 2px;
        border-radius: 4px;
    }
    table {
        background-color: #bbb;
        border-radius: 8px;
    }
    button {
        padding: 4px;
        margin: 4px;
        width: 192px;
        background-color: #ccc;
        border: 1px solid black;
        border-radius: 4px;
        transition: all 0.1s;
    }
    button:hover {
        transform: translateY(-2px);
        background-color: #ddd;
        box-shadow: 0px 6px 4px -2px #00000088;
    }
    button:active {
        transform: translateY(1px);
        background-color: #bbb;
        box-shadow: 0px 0px 4px 0px #00000088;
    }
    td {
        width: 192px;
        height: 18px;
    }
    input {
        width: calc(100% - 4px);
        box-shadow: 0px 0px 4px 1px #00ffaa;
    }
</style>
</html>
)=";