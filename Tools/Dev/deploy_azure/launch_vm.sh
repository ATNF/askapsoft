R_GROUP=RIALTO-2
VM_NAME=azure-builder

#echo "info - Creating resource group"

#az group create --name $R_GROUP --location australiaeast
#echo "info - Group created"

#az vm create \
#    --resource-group $R_GROUP \
#    --name $VM_NAME \
#    --image Canonical:UbuntuServer:18.04-LTS:18.04.201812060 \
#    --generate-ssh-keys \
#    --admin-username askap \
#    --custom-data ./cloud-init-test.txt

IP=`az vm list-ip-addresses --resource-group RIALTO-2 --name azure-builder --output table | tail -1 | awk '{print $2}'`
echo $IP
#az group delete --name $R_GROUP --no-wait --yes
