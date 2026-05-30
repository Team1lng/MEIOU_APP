#!/bin/bash


#####################################################
# 重新链接仓库, 并强制推送
# 注: 因为此项目仅有1人更新, 所以可以使用强制推送和拉取, 并不适用于多人合作的项目


# 解决每次提交都要输入密码的问题  现在终端输入以下指令, 然后进行一次提交
# git config --global credential.helper store
#####################################################

################### Custom parameters start ###################
URL="git@192.168.0.87:sat_qianshi/me_opt_2wdg.git"
################### Custom parameters End ###################

#COLORS
LOG_WHITE() { echo -e "\033[0m"    "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}
LOG_RED()   { echo -e "\033[0;31m" "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}
LOG_GREEN() { echo -e "\033[0;32m" "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}
LOG_YELLO() { echo -e "\033[0;33m" "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}
LOG_BLUE()  { echo -e "\033[0;34m" "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}
LOG_PINK()  { echo -e "\033[0;35m" "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}
LOG_CYAN()  { echo -e "\033[0;36m" "[${FUNCNAME[1]},${BASH_LINENO}] ${@} \033[0m" ;}


_VERSION=1
MSG=""



# 重新链接仓库
re_link(){
    rm  -rf ./.git                   # 删除旧的.git
    git init                        # 初始化
    
    git remote add origin ${URL}
    # git push --set-upstream origin master
    
}



# 强制上传
force_upload(){
    
    git add    -A                       # 添加

    
    git commit -m "force_upload_$1"            # 提交
    git push   --force origin master    # 强制推送到云端
}
#强制下载
force_download(){
    git fetch --all
    git reset --hard origin/master
}


refresh_gitignore(){
    git rm -r --cached .
    git add .
    git commit -m 'update .gitignore'
    git push origin master
}



git_add_tag()
{    


    tag_name=$1
    if [ $# = 0 ];then
        # 提示用户输入 tag 名称
        read -p  "Please enter tag name: "   tag_name

    fi    


    if [ "$2" != "" ];
    then
        force_upload $2
    else
        # 上传之前先推送到最新
        read -p  "commit msg: "   MSG
        force_upload $MSG
    fi

    
    
    
    # 判断 tag 名称是否为空
    if [[ -z "$tag_name" ]]; then
        LOG_RED "Error: tag name cannot be empty."
        exit 1
    fi
    
    # 执行 Git 添加 tag 命令
    git tag -a "$tag_name" -m "Added tag $tag_name $MSG"
    
    # 推送 tag 到远程仓库
    git push origin "$tag_name"
    
    # 输出完成信息
    LOG_WHITE "Tag $tag_name has been added successfully."
    
}

usage(){
    echo "################################### MEUN #####################################"
    echo "$0                MEUN [u|d|i]"
    echo "$0 -upload          force upload        (push)将本地强制覆盖到云端"
    echo "$0 -download        force download      (pull)将云端强制覆盖到本地"
    echo "$0 -relink          re-link             (remote)重新链接云端仓库"
    echo "$0 -ignore          refresh .gitignore  刷新'.gitignore'文件"
    echo "$0 -tag  [msg]      git tag             添加tag并提交tag"
    echo ""
    
    echo "##############################################################################"
    echo ""
}



main(){
    case ${1} in
        -relink|relink)
            re_link
        ;;
        
        -upload|upload)
            force_upload
        ;;
        
        -download|download)
            force_download
        ;;
        
        -ignore|ignore)
            refresh_gitignore
        ;;
        
        -tag|tag)
            git_add_tag ${2} ${3}
        ;;
        
        *)
            usage
        ;;
        
    esac
}

main $1 $2 $3

