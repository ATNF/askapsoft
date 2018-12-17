R_GROUP=RIALTO-2
VM_NAME=azure-runner

echo "info - Creating resource group"

az group create --name $R_GROUP --location australiaeast
echo "info - Group created"

az vm create \
    --resource-group $R_GROUP \
    --name $VM_NAME \
    --image Canonical:UbuntuServer:18.04-LTS:18.04.201812060 \
    --generate-ssh-keys \
    --admin-username askap \

#az vm stop --name azure-runner --resource-group RIALTO-2 
#az group delete --name $R_GROUP --no-wait --yes
