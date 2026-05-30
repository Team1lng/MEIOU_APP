#!/bin/bash


#####################################################
# 重新链接仓库, 并强制推送
# 注: 因为此项目仅有1人更新, 所以可以使用强制推送和拉取, 并不适用于多人合作的项目


# 解决每次提交都要输入密码的问题  现在终端输入以下指令, 然后进行一次提交
# git config --global credential.helper store
#####################################################

################### Custom parameters start ###################
GITLAB_SERVER=""
GITEE_SERVER="https://gitee.com/wu-qianshi/bcom-av100-doorbell.git"

BUILD_TIMESTAMP=$(date "+%m%d%H%M")

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
    
    case ${1} in
        GitLab)
        echo "git remote add origin GitLab ${GITLAB_SERVER}"
            git remote add origin ${GITLAB_SERVER}
        ;;

        Gitee)
            echo "git remote add origin Gitee ${GITEE_SERVER}"
            git remote add origin ${GITEE_SERVER}
        ;;

        *)
            echo "git remote add origin Error!!!!!!!!!!!!"
            echo "git remote add origin Error!!!!!!!!!!!!"
            echo "git remote add origin Error!!!!!!!!!!!!"
            echo "git remote add origin Error!!!!!!!!!!!!"
            echo "git remote add origin Error!!!!!!!!!!!!"
        ;;
    esac
}



# 强制上传
force_upload(){
    
    git add    -A                       # 添加

    
    # git commit -m "force_upload_$BUILD_TIMESTAMP"            # 提交
    git commit -m "update README.md $BUILD_TIMESTAMP"            # 提交
    git push   --force   # 强制推送到云端
}

#强制下载
force_download(){
    git fetch --all
    git reset --hard
}


refresh_gitignore(){
    git rm -r --cached .
    git add .
    git commit -m "update README.md $BUILD_TIMESTAMP"
    git push
}

associated_branch()
{
    REMOTE_BRANCH=$1
    REMOTE_NAME="origin"

# 检查本地分支是否存在
if git rev-parse --verify "$REMOTE_BRANCH" >/dev/null 2>&1; then
    echo "Local branch '$REMOTE_BRANCH' already exists."
    git checkout "$REMOTE_BRANCH"
else
    # 创建本地分支并与远程分支关联
    echo "Creating local branch '$REMOTE_BRANCH' and setting it to track remote branch..."
    git checkout -b "$REMOTE_BRANCH"
fi

    git add    -A                       # 添加
    git commit -m "force_upload_$BUILD_TIMESTAMP"            # 提交

# 检查远程分支是否存在
if git ls-remote --exit-code --heads "$REMOTE_NAME" "$REMOTE_BRANCH" >/dev/null; then
    echo "Remote branch '$REMOTE_BRANCH' already exists."
    git push -f origin $REMOTE_BRANCH
else
    # 创建远程分支
    echo "Creating remote branch '$REMOTE_BRANCH'..."
    git push "$REMOTE_NAME" HEAD:"$REMOTE_BRANCH"
fi

    git branch --set-upstream-to=origin/$REMOTE_BRANCH 
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
    echo "$0 -relink [GitLab/Gitee]         re-link             (remote)重新链接云端仓库"
    echo "$0 -ignore          refresh .gitignore  刷新'.gitignore'文件"
    echo "$0 -tag  [msg]      git tag             添加tag并提交tag"
    echo "$0 -branch  [branch name]      检查分支并创建关联分支"
    echo ""
    
    echo "##############################################################################"
    echo ""
}



main(){
    case ${1} in
        -relink|relink)
            re_link $2
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

        -branch|branch)
            associated_branch ${2}
        ;;
        
        *)
            usage
        ;;
        
    esac
}

main $1 $2 $3

