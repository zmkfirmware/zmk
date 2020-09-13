$ErrorActionPreference = "Stop"

function Get-Choice-From-Options {
    param(
        [String[]] $Options,
        [String] $Prompt
    )

    while ($true) {
        for ($i = 0; $i -lt $Options.length; $i++) {
            Write-Host "$($i + 1)) $($Options[$i])"
        }
        
        Write-Host "$($Options.length + 1)) Quit"
        $selection = Read-Host $Prompt

        if ($selection -eq $Options.length + 1) {
            Write-Host "Goodbye!"
            exit
        }
        elseif ($selection -le $Options.length) {
            $choice = $($selection - 1)
            break
        }
        else {
            Write-Host "Invalid Option. Try another one."
        }
    }

    return $choice
}

function Test-Git-Config {
    param(
        [String] $Option,
        [String] $ErrMsg
    )

    git config $Option | Out-Null

    if ($lastExitCode -ne 0) {
        Write-Host $ErrMsg
        exit
    }
}

try {
    git | Out-Null
}
catch [System.Management.Automation.CommandNotFoundException] {
    Write-Host "Git is not installed, and is required for this script!"
    exit
}

Test-Git-Config -Option "user.name" -ErrMsg "Git username not set!`nRun: git config --global user.name 'My Name'"
Test-Git-Config -Option "user.email" -ErrMsg "Git email not set!`nRun: git config --global user.name 'example@myemail.com'"

$repo_path = "https://github.com/zmkfirmware/zmk-config-split-template.git"

$title = "ZMK Config Setup:"
$prompt = "Pick an MCU board"
$options = "nice!nano", "QMK Proton-C", "BlueMicro840 (v1)"
$boards = "nice_nano", "proton_c", "bluemicro840_v1"

Write-Host "$title"
Write-Host ""
Write-Host "MCU Board Selection:"

$choice = Get-Choice-From-Options -Options $options -Prompt $prompt
$board = $($boards[$choice])

Write-Host ""
Write-Host "Keyboard Shield Selection:"
$prompt = "Pick a keyboard"

# TODO: Add support for "Other" and linking to docs on adding custom shields in user config repos.
$options = "Kyria", "Lily58", "Corne", "Splitreus62", "Sofle", "Iris", "RoMac"
$names = "kyria", "lily58", "corne", "splitreus62", "sofle", "iris", "romac"
$splits = "y", "y", "y", "y", "y", "y", "n"

$choice = Get-Choice-From-Options -Options $options -Prompt $prompt
$shield_title = $($options[$choice])
$shield = $($names[$choice])
$split = $($splits[$choice])

if ($split -eq "n") {
    $repo_path = "https://github.com/zmkfirmware/zmk-config-template.git"
}

$copy_keymap = Read-Host "Copy in the stock keymap for customisation? [Yn]"

if ($copy_keymap -eq "" -or $copy_keymap -eq "Y" -or $copy_keymap -eq "y") {
    $copy_keymap = "yes"
}

$github_user = Read-Host "GitHub Username (leave empty to skip GitHub repo creation)"

if ($github_user -ne "") {
    $repo_name = Read-Host "GitHub Repo Name [zmk-config]"

    if ($repo_name -eq "") {
        $repo_name = "zmk-config"
    }

    $github_repo = Read-Host "GitHub Repo [https://github.com/$github_user/$repo_name.git]"
    
    if ($github_repo -eq "") {
        $github_repo = "https://github.com/$github_user/$repo_name.git"
    }
}
else {
    $repo_name = "zmk-config"
    $github_repo = ""
}

Write-Host ""
Write-Host "Preparing a user config for:"
Write-Host "* MCU Board: ${board}"
Write-Host "* Shield: ${shield}"

if ($copy_keymap -eq "yes") {
    Write-Host "* Copy Keymap?: Yes"
}
else {
    Write-Host "* Copy Keymap?: No"
}

if ($github_repo -ne "") {
    Write-Host "* GitHub Repo to Push (please create this in GH first!): $github_repo"
}

Write-Host ""
$do_it = Read-Host "Continue? [Yn]"

if ($do_it -ne "" -and $do_it -ne "Y" -and $do_it -ne "y") {
    Write-Host "Aborting..."
    exit
}

git clone --single-branch "$repo_path" "$repo_name"
Set-Location "$repo_name"

Push-Location config

Invoke-RestMethod -Uri "https://raw.githubusercontent.com/zmkfirmware/zmk/main/app/boards/shields/${shield}/${shield}.conf" -OutFile "${shield}.conf"

if ($copy_keymap -eq "yes") {
    Invoke-RestMethod -Uri "https://raw.githubusercontent.com/zmkfirmware/zmk/main/app/boards/shields/${shield}/${shield}.keymap" -OutFile "${shield}.keymap"
}

Pop-Location

$build_file = (Get-Content .github/workflows/build.yml).replace("BOARD_NAME", $board)
$build_file = $build_file.replace("SHIELD_NAME", $shield)
$build_file = $build_file.replace("KEYBOARD_TITLE", $shield_title)

if ($board -eq "proton_c") {
    $build_file = $build_file.replace("uf2", "hex")
}

Set-Content -Path .github/workflows/build.yml -Value $build_file

Remove-Item -Recurse -Force .git
git init .
git add .
git commit -m "Initial User Config."

if ($github_repo -ne "") {
    git remote add origin "$github_repo"
    git push --set-upstream origin $(git symbolic-ref --short HEAD)

    if ($github_repo -imatch "https") {
        $actions = "$($github_repo.substring(0, $github_repo.length - 4))/actions"
        Write-Host "Your firmware should be availalbe from the GitHub Actions shortly: $actions"
    }
}
